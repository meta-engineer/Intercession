#include "server_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"
#include "staging/cosmos_builder.h"
#include "staging/client_focal_entity.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(std::shared_ptr<EventBroker> sharedBroker, TimelineApi localTimelineApi)
        : I_NetworkDynamo(sharedBroker)
        , m_timelineApi(localTimelineApi)
        , m_networkApi(m_timelineApi.get_port())
    {
        PLEEPLOG_TRACE("Start server networking pipeline setup");

        // setup relays

        // start listening on asio server
        m_networkApi.start();

        // setup handlers
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ServerNetworkDynamo::_entity_created_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ServerNetworkDynamo::_timestream_interception_handler));

        PLEEPLOG_TRACE("Done server networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        // clear handlers
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ServerNetworkDynamo::_entity_created_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ServerNetworkDynamo::_timestream_interception_handler));
    }
    
    void ServerNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // Handle async data from network and local (timelineApi)
        // handling paradigms should ideally be the same:
        // let their respective threads accumulate messages,
        // then during run_relays we will pop them and handle them here

        // Network/Local Api should only handle specific internal logic for themselves
        // which they can do so immediately on their own thread
        //   (e.g. managing connections)
        // And then store necessary data for us to invoke cosmos specific logic later
        //   (e.g. submittions to relays, or events)
        // dynamo could call with message data/event as a "packet" (though not the strict definition of a Cosmos packet)

        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) cosmos = nullptr;

        if (!cosmos)
        {
            //PLEEPLOG_WARN("Relays cannot do any meaningful work with a null Cosmos");
            // don't return so that we can actively clear out incoming messages
        }

        uint16_t currentCoherency = 0;
        if (cosmos) currentCoherency = cosmos->get_coherency();
        // sync coherency with child and clients, every ~5 seconds
        if (currentCoherency % 360 == 0)
        {
            EventMessage syncMessage(events::network::COHERENCY_SYNC, currentCoherency);
            events::network::COHERENCY_SYNC_params syncParams = { m_timelineApi.get_timeslice_id() };
            syncMessage << syncParams;

            if (m_timelineApi.has_past())
            {
                // timesliceId increases into the past
                m_timelineApi.send_message(m_timelineApi.get_timeslice_id() + 1, syncMessage);
            }

            m_networkApi.broadcast_message(syncMessage);
        }

        // Ordering of methods?
        // First: process DIRECT messages from other timeslices
        //_process_timeline_messages();
        EventMessage msg;
        while(m_timelineApi.pop_message(msg))
        {
            // No message handlers whos side effects don't rely on modifying the cosmos
            if (!cosmos) continue;

            switch (msg.header.id)
            {
            case events::cosmos::ENTITY_CREATED:
            {
                // Someone telling me that an entity has been created
                // NOT to create one ourselves, that will be received over the timeSTREAM
                events::cosmos::ENTITY_CREATED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_CREATED notification for Entity " + std::to_string(data.entity));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_CREATED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    cosmos->increment_hosted_entity_count(data.entity);
                }
            }
            break;
            case events::cosmos::ENTITY_REMOVED:
            {
                // Someone telling me that an entity has been deleted
                // NOT to delete one ourselves, that will be received over the timeSTREAM
                events::cosmos::ENTITY_REMOVED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_REMOVED notification for Entity " + std::to_string(data.entity));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_REMOVED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    cosmos->decrement_hosted_entity_count(data.entity);
                }
            }
            break;
            case events::network::COHERENCY_SYNC:
            {
                events::network::COHERENCY_SYNC_params data;
                msg >> data;
                uint16_t targetCoherency = msg.header.coherency -
                    static_cast<uint16_t>(m_timelineApi.get_timeslice_delay() * m_timelineApi.get_simulation_hz());

                if (targetCoherency != cosmos->get_coherency())
                {
                    PLEEPLOG_DEBUG("Detected Coherency Desync! COHERENCY_SYNC notification from slice " + std::to_string(data.senderId) + " of " + std::to_string(targetCoherency) + " does not match my coherency " + std::to_string(cosmos->get_coherency()));

                    cosmos->set_coherency(targetCoherency);
                }
            }
            break;
            case events::cosmos::TIMESTREAM_INTERCEPTION:
            {
                // we are signalled that an interception has occurred sometime/where from another timeslice
                // if we have a future version of that entity, then set it to be "superposition"
                events::cosmos::TIMESTREAM_INTERCEPTION_params data;
                msg >> data;

                CausalChainlink link = derive_causal_chain_link(data.recipient);
                if (link <= 0)
                {
                    PLEEPLOG_ERROR("Something went wrong, a link-0 entity (" + std::to_string(data.recipient) + ") cannot have been interrupted. Ignoring...");
                    break;
                }

                TimesliceId host = derive_timeslice_id(data.recipient);
                GenesisId genesis = derive_genesis_id(data.recipient);

                // ASSUME interception happened in our direct child, therefore
                // check for entity with 1 less causalchainlink than the reported recipient
                Entity presentRecipient = compose_entity(host, genesis, link - 1);

                if (!cosmos->entity_exists(presentRecipient))
                {
                    PLEEPLOG_WARN("Future of intercepted entity (" + std::to_string(presentRecipient) + ") does not exist. Message may not have been sent from direct child? Ignoring...");
                    break;
                }

                
                // Put the entity into a superposition
                // if recipient has no spacetime component, add one
                if (!cosmos->has_component<SpacetimeComponent>(presentRecipient))
                {
                    SpacetimeComponent newSpacetime;
                    cosmos->add_component<SpacetimeComponent>(presentRecipient, newSpacetime);
                }
                
                SpacetimeComponent& oldSpacetime = cosmos->get_component<SpacetimeComponent>(presentRecipient);
                oldSpacetime.timestreamState = TimestreamState::superposition;
                oldSpacetime.timestreamStateCoherency = cosmos->get_coherency();

                // Forked state will stop further reading from timestream so we dont need to clear it
                //m_timelineApi.clear_past_timestream(data.recipient);
                // TODO: Store timestream at this moment or at moment of resolution for restoring upon a paradox resolution?


                // pass interception notification up the chain
                if (m_timelineApi.has_future())
                {
                    data.recipient = presentRecipient;
                    msg << data;

                    // timesliceId decreases into the future
                    assert(m_timelineApi.get_timeslice_id() > 0);
                    m_timelineApi.send_message(m_timelineApi.get_timeslice_id() - 1, msg);
                }
            }
            break;
            case events::network::JUMP_REQUEST:
            {
                PLEEPLOG_DEBUG("Received jump request from a server");

                events::network::JUMP_REQUEST_params jumpInfo;
                msg >> jumpInfo;
                msg << jumpInfo;
                
                // generate transfer code for client
                uint32_t timeSeed = static_cast<uint32_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
                );
                uint32_t transferCode = Squirrel3(jumpInfo.entity, timeSeed);
                while(m_transferCache.count(transferCode))
                {
                    PLEEPLOG_ERROR("Generated transfer code is already taken, trying again...");
                    timeSeed--;
                    transferCode = Squirrel3(jumpInfo.entity, timeSeed);
                }

                // save entity in preparation for client to connect soon
                m_transferCache.insert({transferCode, msg});

                // return good jump response
                EventMessage responseMsg(events::network::JUMP_RESPONSE);
                events::network::JUMP_RESPONSE_params responseInfo{
                    jumpInfo.requesterConnId,
                    transferCode,
                    m_timelineApi.get_port()
                };
                responseMsg << responseInfo;

                // return to sender
                int source = m_timelineApi.get_timeslice_id() - jumpInfo.timesliceDelta;
                m_timelineApi.send_message(static_cast<TimesliceId>(source), responseMsg);
            }
            break;
            case events::network::JUMP_RESPONSE:
            {
                PLEEPLOG_DEBUG("Received jump response from a server");

                events::network::JUMP_RESPONSE_params jumpInfo;
                msg >> jumpInfo;
                msg << jumpInfo;

                // forward message to jump requesting client
                PLEEPLOG_DEBUG("Forwarding to " + std::to_string(jumpInfo.requesterConnId));
                m_networkApi.send_message(jumpInfo.requesterConnId, msg);
            }
            break;
            case events::cosmos::CONDEMN_ENTITY:
            {
                PLEEPLOG_DEBUG("Received explicit condemn from a server");
                // another timeslice has called for us to destroy an entity
                // this is likely after a successfull timeslice jump
                // forward it to our cosmos
                
                events::cosmos::CONDEMN_ENTITY_params condemnInfo;
                msg >> condemnInfo;

                cosmos->condemn_entity(condemnInfo.entity);
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Received unknown message id " + std::to_string(msg.header.id));
            }
            }
        }

        // Second: handle incoming timestream messages from other timeslices
        // This should get our cosmos as close as possible to realtime
        //_process_timestream_messages();
        if (m_timelineApi.has_future())
        {
            // we want to iterate through all entries in the future EntityTimestreamMap
            // not necessarily our cosmos' entities...
            std::vector<Entity> availableEntities = m_timelineApi.get_entities_with_future_streams();

            for (Entity& entity : availableEntities)
            {
                EventMessage evnt(1);
                // timeline api will return false if nothing available at currentCoherency or earlier
                while (m_timelineApi.pop_future_timestream(entity, currentCoherency, evnt))
                {
                    // continue to clear out messages if no working cosmos
                    if (!cosmos) continue;

                    // if entity's timstream state is forked then ignore future timestream
                    if (cosmos->has_component<SpacetimeComponent>(entity) &&
                        cosmos->get_component<SpacetimeComponent>(entity).timestreamState == TimestreamState::forked)
                    {
                        continue;
                    }

                    // Handle timestream messages just as a client would, i think?
                    switch(evnt.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        evnt >> updateInfo;
                        //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());

                        // ensure entity exists
                        if (cosmos->entity_exists(updateInfo.entity))
                        {
                            // read update into Cosmos
                            cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, updateInfo.subset, evnt);
                        }
                        else
                        {
                            PLEEPLOG_ERROR("Received ENTITY_UPDATE for entity " + std::to_string(updateInfo.entity) + " which does not exist, skipping...");
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        // register entity and create components to match signature
                        events::cosmos::ENTITY_CREATED_params createInfo;
                        evnt >> createInfo;
                        PLEEPLOG_DEBUG("Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                        if (cosmos->register_entity(createInfo.entity))
                        {
                            for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                            {
                                if (createInfo.sign.test(c)) cosmos->add_component(createInfo.entity, c);
                            }
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_REMOVED:
                    {
                        // call cosmos to delete entity, it will emit event again over broker and cause host decrement
                        // (sender SHOULD have correctly offset causalchainlink already)
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        evnt >> removeInfo;
                        PLEEPLOG_TRACE("Remove Entity: " + std::to_string(removeInfo.entity));

                        // use condemn event to avoid double deletion
                        cosmos->condemn_entity(removeInfo.entity);
                    }
                    break;
                    default:
                    {
                        // reached end of this entities timestream
                        continue;
                    }
                    }
                }
            }
        }

        // Third: handle incoming client messages from network
        //_process_network_messages();
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        net::OwnedMessage<EventId> remoteMsg;
        while ((messageCount++) < maxMessages && m_networkApi.pop_message(remoteMsg))
        {
            // We could want to respond to a client, so even if we have no working cosmos we need to run handlers (safely)

            switch (remoteMsg.msg.header.id)
            {
            case events::cosmos::ENTITY_UPDATE:
            {
                //PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received entity update message");

                events::cosmos::ENTITY_UPDATE_params updateInfo;
                remoteMsg.msg >> updateInfo;
                // TODO: Check updated entity matches client's assigned entity

                // We should only be receiving upstream components...
                if (cosmos) cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, true, remoteMsg.msg);
            }
            break;
            case events::network::NEW_CLIENT:
            {
                // this is the first message a client will send us after connection
                PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received new client notice");

                events::network::NEW_CLIENT_params clientInfo;
                remoteMsg.msg >> clientInfo;

                // TODO: Send Cosmos config
                // should be stateless and scan current workingCosmos?
                // How should scan work, just store config into Cosmos when it is built? What if it changes afterwards?
                // If CosmosBuilder could use synchro/component typeid then we could scan Cosmos' registries
                // CosmosBuilder::Config needs to be serializable... are std::sets serializable?

                // if we have no cosmos at this point we can't proceed
                // maybe we should deny client connections if there is no cosmos?
                // assert(cosmos);
                if (!cosmos) break;
                
                PLEEPLOG_WARN("New client joined, I should send them a cosmos config!");
                remoteMsg.remote->enable_sending();
                /* Message<EventId> configMsg(events::cosmos::CONFIG);
                events::cosmos::CONFIG_params configInfo;
                CosmosBuilder scanner;
                configInfo.config = scanner.scan(cosmos);
                configMsg << configInfo;
                remoteMsg.remote->send(configMsg); */

                PLEEPLOG_DEBUG("Sending Entities to new client");
                // Send initialization for all entities that already exist
                for (auto signIt : cosmos->get_signatures_ref())
                {
                    EventMessage createMsg(events::cosmos::ENTITY_CREATED, currentCoherency);
                    events::cosmos::ENTITY_CREATED_params createInfo = {signIt.first, signIt.second};
                    createMsg << createInfo;
                    //PLEEPLOG_DEBUG("Sending message: " + createMsg.info());
                    remoteMsg.remote->send(createMsg);
                }

                
                // check transfer codes for available entity
                if (clientInfo.transferCode != 0 && m_transferCache.count(clientInfo.transferCode))
                {
                    // fetch serialized entity data cache from previous JUMP_REQUEST
                    EventMessage jumpMsg = m_transferCache.at(clientInfo.transferCode);
                    m_transferCache.erase(clientInfo.transferCode);
                    events::network::JUMP_REQUEST_params jumpInfo;
                    jumpMsg >> jumpInfo;

                    if (cosmos->register_entity(jumpInfo.entity))
                    {
                        clientInfo.entity = jumpInfo.entity;
        
                        cosmos->deserialize_entity_components(jumpInfo.entity, jumpInfo.sign, false, jumpMsg);
                        PLEEPLOG_DEBUG("Registered entity from transfer cache " + std::to_string(jumpInfo.entity) + " for client " + std::to_string(remoteMsg.remote->get_id()));

                        // signal to host to incrememnt host count (like create_entity does locally)
                        // (this is in addition to host increment in event handler if we have a past)
                        EventMessage createMsg(events::cosmos::ENTITY_CREATED);
                        events::cosmos::ENTITY_CREATED_params createInfo{
                            jumpInfo.entity,
                            jumpInfo.sign
                        };
                        createMsg << createInfo;
                        m_timelineApi.send_message(derive_timeslice_id(createInfo.entity), createMsg);

                        // signal jump source timeslice to delete its copy of entity
                        // (as long as the above createMsg is received first, host count will not 0 out)
                        EventMessage condemnMsg(events::cosmos::CONDEMN_ENTITY);
                        events::cosmos::CONDEMN_ENTITY_params condemnInfo{
                            jumpInfo.entity
                        };
                        condemnMsg << condemnInfo;
                        int source = m_timelineApi.get_timeslice_id() - jumpInfo.timesliceDelta;
                        m_timelineApi.send_message(static_cast<TimesliceId>(source), condemnMsg);
                    }
                    else
                    {
                        PLEEPLOG_ERROR("Registry of transferred entity " + std::to_string(jumpInfo.entity) + " failed");
                        clientInfo.entity = NULL_ENTITY;
                    }
                }
                else
                {
                    // Server will create client character and then pass it its Entity
                    // Client may have to do predictive entity creation in the future,
                    // but we'll avoid that here for now because client has to wait anyways
                    clientInfo.entity = create_client_focal_entity(cosmos, m_sharedBroker);
                    PLEEPLOG_DEBUG("Created new entity " + std::to_string(clientInfo.entity) + " for client " + std::to_string(remoteMsg.remote->get_id()));
                }

                // send client upstream signture update for its focal entity
                EventMessage updateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params updateInfo = {
                    clientInfo.entity,
                    cosmos->get_entity_signature(clientInfo.entity) & cosmos->get_category_signature(ComponentCategory::upstream),
                    true
                };
                cosmos->serialize_entity_components(updateInfo.entity, updateInfo.sign, updateMsg);

                updateMsg << updateInfo;
                remoteMsg.remote->send(updateMsg);

                // Forward new client message to signal for cosmos to initialize client side entities
                PLEEPLOG_DEBUG("Sending new client acknowledgement to new client");
                remoteMsg.msg.header.coherency = currentCoherency;
                remoteMsg.msg << clientInfo;
                remoteMsg.remote->send(remoteMsg.msg);

                // TODO: we should also keep track of the clientInfo.entity
                // (store it in remoteMsg.remote?)
                // when we receive input updates from this client, we should only allow
                // and/or assume they are for this entity only!
                
            }
            break;
            case events::network::JUMP_REQUEST:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received jump request from a client");

                // lead local client connection id into message
                events::network::JUMP_REQUEST_params jumpInfo;
                remoteMsg.msg >> jumpInfo;
                jumpInfo.requesterConnId = remoteMsg.remote->get_id();
                remoteMsg.msg << jumpInfo;

                // send to server based on timesliceDelta
                int destination = m_timelineApi.get_timeslice_id() + jumpInfo.timesliceDelta;

                // ensure destination is in range
                if (destination < 0 || destination > m_timelineApi.get_num_timeslices() - 1)
                {
                    // create error response for client
                    EventMessage errorMsg(events::network::JUMP_RESPONSE);
                    events::network::JUMP_RESPONSE_params errorInfo{
                        0, 0, 0
                    };
                    errorMsg << errorInfo;
                    remoteMsg.remote->send(errorMsg);
                    break;
                }

                // good to send now
                m_timelineApi.send_message(static_cast<TimesliceId>(destination), remoteMsg.msg);
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received unknown message");
            }
            }
        }

        // Fourth: Process all submitted data in relays

        // Fifth: After all ingesting is done, send/broadcast fresh downstream data to clients & children
        if (cosmos)
        {
            for (auto signIt : cosmos->get_signatures_ref())
            {
                // push to clients 
                Signature downstreamSign = cosmos->get_category_signature(ComponentCategory::downstream);
                EventMessage clientUpdateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params clientUpdateInfo = {
                    signIt.first,
                    signIt.second & downstreamSign, // only downstream components
                    true
                };
                cosmos->serialize_entity_components(clientUpdateInfo.entity, clientUpdateInfo.sign, clientUpdateMsg);

                clientUpdateMsg << clientUpdateInfo;
                m_networkApi.broadcast_message(clientUpdateMsg);


                // and push to child timestream (except if there is not past to push to)
                if (!m_timelineApi.has_past()) continue;

                EventMessage childUpdateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params childUpdateInfo = {
                    signIt.first,
                    signIt.second,  // full signature of components
                    false
                };
                cosmos->serialize_entity_components(childUpdateInfo.entity, childUpdateInfo.sign, childUpdateMsg);
                
                // increment chain link (increase) when moving into the past
                increment_causal_chain_link(childUpdateInfo.entity);
                childUpdateMsg << childUpdateInfo;
                m_timelineApi.push_past_timestream(childUpdateInfo.entity, childUpdateMsg);
            }
        }
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        // clear our working cosmos
        m_workingCosmos.reset();

        // clear relay working cosmos and leftover submittions
    }

    void ServerNetworkDynamo::submit(CosmosAccessPacket data) 
    {
        // save our own working cosmos for our event handling
        m_workingCosmos = data.owner;

        // pass working cosmos to relays
    }
    
    TimesliceId ServerNetworkDynamo::get_timeslice_id()
    {
        return m_timelineApi.get_timeslice_id();
    }
    
    size_t ServerNetworkDynamo::get_num_connections()
    {
        return m_networkApi.get_num_connections();
    }
    
    void ServerNetworkDynamo::_entity_created_handler(EventMessage entityEvent)
    {   
        // Broadcast creation event to clients, the run_relays update will populate it
        m_networkApi.broadcast_message(entityEvent);

        events::cosmos::ENTITY_CREATED_params newEntityParams;
        entityEvent >> newEntityParams;
        
        //PLEEPLOG_DEBUG("Handling ENTITY_CREATED event for entity " + std::to_string(newEntityParams.entity));

        // Entity cannot be created in a non-normal timestreamState?

        // if entity was created locally, its host count starts at 1
        // if entity was send from the future, it will have been incrememnted when it entered our future timestream

        // If we have a past timeslice, that means next frame this entity updates will enter the past timestream
        if (m_timelineApi.has_past())
        {
            events::cosmos::ENTITY_CREATED_params propogateNewEntityParams = newEntityParams;
            increment_causal_chain_link(propogateNewEntityParams.entity);
            entityEvent << propogateNewEntityParams;

            m_timelineApi.push_past_timestream(propogateNewEntityParams.entity, entityEvent);

            // signal for host to increment now that entity is in the past
            TimesliceId host = derive_timeslice_id(propogateNewEntityParams.entity);
            m_timelineApi.send_message(host, entityEvent);
        }
    }
    
    void ServerNetworkDynamo::_entity_removed_handler(EventMessage entityEvent)
    {
        //PLEEPLOG_DEBUG("Handling ENTITY_REMOVED event");
        // REMEMBER: this is called AFTER the entity was destroyed so you can't lookup any of its data

        m_networkApi.broadcast_message(entityEvent);

        events::cosmos::ENTITY_REMOVED_params removedEntityParams;
        entityEvent >> removedEntityParams;
        entityEvent << removedEntityParams; // re-fill message for forwarding

        // entity has left our timeslice so we need to decrement its host's count
        TimesliceId host = derive_timeslice_id(removedEntityParams.entity);
        // Don't send to ourselves, decrement will already have happened
        if (host != m_timelineApi.get_timeslice_id()) m_timelineApi.send_message(host, entityEvent);

        // propagate further down the timeline
        if (m_timelineApi.has_past())
        {
            events::cosmos::ENTITY_REMOVED_params propogateRemovedEntityParams;
            entityEvent >> propogateRemovedEntityParams;
            increment_causal_chain_link(propogateRemovedEntityParams.entity);
            entityEvent << propogateRemovedEntityParams;

            m_timelineApi.push_past_timestream(propogateRemovedEntityParams.entity, entityEvent);

            // once child receives the above event, it will signal another decrement
        }
    }
    
    void ServerNetworkDynamo::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        PLEEPLOG_DEBUG("Handling TIMESTREAM_INTERCEPTION event");

        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;
        interceptionEvent << interceptionInfo;

        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();

        // Put the entity into a forked timestream state (stop receiving from future)
        // if recipient has no spacetime component, add one
        if (!cosmos->has_component<SpacetimeComponent>(interceptionInfo.recipient))
        {
            SpacetimeComponent newSpacetime;
            cosmos->add_component<SpacetimeComponent>(interceptionInfo.recipient, newSpacetime);
        }
        
        SpacetimeComponent& oldSpacetime = cosmos->get_component<SpacetimeComponent>(interceptionInfo.recipient);
        oldSpacetime.timestreamState = TimestreamState::forked;
        oldSpacetime.timestreamStateCoherency = cosmos->get_coherency();

        // Signal to future timeslice to put recipient into superposition timestream state
        // Timestream forked state should restrict reading from the timestream from our side

        PLEEPLOG_INFO("Detected interaction event from entity " + std::to_string(interceptionInfo.agent) + " to " + std::to_string(interceptionInfo.recipient));

        // We also need to be able to detect knock-on interactions an object with a non-zero chainlink might cause after being modified by a chainlink zero entity, before and after superposition resolution

        if (m_timelineApi.has_future())
        {
            // timesliceId decreases into the future
            assert(m_timelineApi.get_timeslice_id() > 0);
            m_timelineApi.send_message(m_timelineApi.get_timeslice_id() - 1, interceptionEvent);
        }
    }
}