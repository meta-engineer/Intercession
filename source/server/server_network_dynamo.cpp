#include "server_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"
#include "staging/cosmos_builder.h"
#include "staging/client_focal_entity.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker, TimelineApi localTimelineApi)
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
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_MODIFIED, ServerNetworkDynamo::_entity_modified_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::INTERCESSION, ServerNetworkDynamo::_intercession_handler));

        PLEEPLOG_TRACE("Done server networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        // clear handlers
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ServerNetworkDynamo::_entity_created_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_MODIFIED, ServerNetworkDynamo::_entity_modified_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::INTERCESSION, ServerNetworkDynamo::_intercession_handler));
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
        while(m_timelineApi.is_message_available())
        {
            EventMessage msg = m_timelineApi.pop_message();
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
                //PLEEPLOG_DEBUG("Received ENTITY_CREATED message for Entity " + std::to_string(data.entity));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_CREATED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    cosmos->increment_hosted_temporal_entity_count(data.entity);
                }
            }
            break;
            case events::cosmos::ENTITY_REMOVED:
            {
                events::cosmos::ENTITY_REMOVED_params data;
                msg >> data;
                PLEEPLOG_DEBUG("Received ENTITY_REMOVED message for Entity " + std::to_string(data.entity));
                // double check if we are it's host
                if (derive_timeslice_id(data.entity) != m_timelineApi.get_timeslice_id())
                {
                    PLEEPLOG_WARN("Received a ENTITY_REMOVED signal for an entity we don't host. Is this intentional?");
                }
                else
                {
                    cosmos->decrement_hosted_temporal_entity_count(data.entity);
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
                    PLEEPLOG_DEBUG("Detected Coherency Desync! COHERENCY_SYNC message from slice " + std::to_string(data.senderId) + " of " + std::to_string(targetCoherency) + " does not match my coherency " + std::to_string(cosmos->get_coherency()));

                    cosmos->set_coherency(targetCoherency);
                }
            }
            break;
            case events::network::INTERCESSION:
            {
                // we are signalled that an intercession has occurred sometime/where
                events::network::INTERCESSION_params data;
                msg >> data;

                CausalChainlink link = derive_causal_chain_link(data.intercessee);
                assert(link > 0);
                //PLEEPLOG_ERROR("Something went wrong, a link-0 entity (" + std::to_string(data.intercessee) + ") cannot have been interceded.");

                TimesliceId host = derive_timeslice_id(data.intercessee);
                GenesisId genesis = derive_genesis_id(data.intercessee);

                // ASSUME intercession happened in our direct child, therefore
                // check for entity with 1 less causalchainlink than the reported intercessee
                Entity presentIntercesee = compose_entity(host, genesis, link - 1);

                // Put the entity into a superposition
                // superposition is constrained to physical objects, so if it is not physical
                //   an unexpected entity has been interceeded
                try
                {
                    TransformComponent& trans = cosmos->get_component<TransformComponent>(presentIntercesee);
                    trans.superposition = true;
                }
                catch(const std::exception& e)
                {
                    PLEEPLOG_ERROR(e.what());
                    PLEEPLOG_ERROR("Could not fetch TransformComponent for interceeded entity " + std::to_string(presentIntercesee) + ". Superpositions can only be applied to physical entities, ignoring this intercession event.");
                    break;
                }
                
                // Superposition will stop any further pushes to timestream so we can clear it
                // TODO: Store timestream for resetting upon a paradox
                m_timelineApi.clear_past_timestream(data.intercessee);

                // TEMP: Resolve superposition by just deleting it?
                // Signal for intercessee to be cleaned up when Cosmos is safe
                EventMessage condemnMsg(events::cosmos::CONDEMN_ENTITY);
                events::cosmos::CONDEMN_ENTITY_params condemnInfo{
                    presentIntercesee
                };
                condemnMsg << condemnInfo;
                //m_sharedBroker->send_event(condemnMsg);
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Received unknown message id " + std::to_string(msg.header.id));
            }
            break;
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
                EventMessage msg(1);
                // timeline api will return message with id = 0 if nothing is available
                while (msg.header.id != 0)
                {
                    msg = m_timelineApi.pop_future_timestream(entity, currentCoherency);

                    // continue to clear out messages if no working cosmos
                    if (!cosmos) continue;

                    // Handle timestream messages just as a client would, i think?
                    switch(msg.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        //PLEEPLOG_TRACE("Received ENTITY_UPDATE message from future");

                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        msg >> updateInfo;
                        //PLEEPLOG_DEBUG(std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());
                        //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity));

                        // confirm entity exists?
                        // confirm entity signatures match?

                        // read update into Cosmos
                        cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, msg);
                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        //PLEEPLOG_TRACE("Received ENTITY_CREATED message from future");

                        // register entity and create components to match signature
                        events::cosmos::ENTITY_CREATED_params createInfo;
                        msg >> createInfo;
                        //PLEEPLOG_DEBUG(std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                        if (cosmos->register_entity(createInfo.entity))
                        {
                            for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                            {
                                if (createInfo.sign.test(c)) cosmos->add_component(createInfo.entity, c);
                            }
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_MODIFIED:
                    {
                        //PLEEPLOG_TRACE("Received ENTITY_MODIFIED message from future");

                        // create any new components, remove any components message does not have
                        events::cosmos::ENTITY_MODIFIED_params modInfo;
                        msg >> modInfo;
                        //PLEEPLOG_DEBUG(std::to_string(modInfo.entity) + " | " + modInfo.sign.to_string());

                        // ensure entity exists
                        if (cosmos->entity_exists(modInfo.entity))
                        {
                            Signature entitySign = cosmos->get_entity_signature(modInfo.entity);

                            for (ComponentType c = 0; c < modInfo.sign.size(); c++)
                            {
                                if (modInfo.sign.test(c) && !entitySign.test(c)) cosmos->add_component(modInfo.entity, c);

                                if (!modInfo.sign.test(c) && entitySign.test(c)) cosmos->remove_component(modInfo.entity, c);
                            }
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_REMOVED:
                    {
                        PLEEPLOG_TRACE("Received ENTITY_REMOVED message from future");

                        // call cosmos to delete entity, it will emit event again over broker and cause host decrement
                        // (sender SHOULD have correctly offset causalchainlink already)
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        msg >> removeInfo;

                        // use condemn event to avoid double deletion
                        EventMessage condemnMsg(events::cosmos::CONDEMN_ENTITY);
                        events::cosmos::CONDEMN_ENTITY_params condemnInfo{
                            removeInfo.entity
                        };
                        condemnMsg << condemnInfo;
                        m_sharedBroker->send_event(condemnMsg);
                    }
                    break;
                    default:
                        // reached end of this entities timestream
                        continue;
                    }
                }
            }

            //if (msg.header.id == 0) noop;
        }

        // Third: handle incoming client messages from network
        //_process_network_messages();
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        while ((messageCount++) < maxMessages && m_networkApi.is_message_available())
        {
            net::OwnedMessage<EventId> msg = m_networkApi.pop_message();
            // We could want to respond to a client, so even if we have no working cosmos we need to run handlers (safely)

            switch (msg.msg.header.id)
            {
            case events::network::INTERCESSION:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Bouncing intercession update message");
                // just bounce back
                msg.remote->send(msg.msg);
            }
            break;
            case events::cosmos::ENTITY_UPDATE:
            {
                //PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received entity update message");

                events::cosmos::ENTITY_UPDATE_params updateInfo;
                msg.msg >> updateInfo;
                // TODO: Check updated entity matches client's assigned entity

                // We should only be receiving upstream components...
                if (cosmos) cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, msg.msg);
            }
            break;
            case events::network::NEW_CLIENT:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received new client notice");

                // new client entity value is not real yet, I_Server generated this event
                events::network::NEW_CLIENT_params clientInfo;
                msg.msg >> clientInfo;

                // TODO: send app_info? then intercession_app_info?

                // Send Cosmos config
                // should be stateless and scan current workingCosmos?
                // How should scan work, just store config into Cosmos when it is built? What if it changes afterwards?
                // If CosmosBuilder could use synchro/component typeid then we could scan Cosmos' registries
                // CosmosBuilder::Config needs to be serializable... are std::sets serializable?

                PLEEPLOG_DEBUG("New client joined, I should send them a cosmos config!");
                // if we have no cosmos at this point we can't proceed
                // maybe we should deny client connections if there is no cosmos?
                // assert(cosmos);
                if (!cosmos) break;
                /* Message<EventId> configMsg(events::cosmos::CONFIG);
                events::cosmos::CONFIG_params configInfo;
                CosmosBuilder scanner;
                configInfo.config = scanner.scan(cosmos);
                configMsg << configInfo;
                msg.remote->send(configMsg); */

                PLEEPLOG_DEBUG("Sending Entities to new client");
                // Send initialization for all entities that already exist
                for (auto signIt : cosmos->get_signatures_ref())
                {
                    EventMessage createMsg(events::cosmos::ENTITY_CREATED, currentCoherency);
                    events::cosmos::ENTITY_CREATED_params createInfo = {signIt.first, signIt.second};
                    createMsg << createInfo;
                    //PLEEPLOG_DEBUG("Sending message: " + createMsg.info());
                    msg.remote->send(createMsg);
                }
                
                // Server will create client character and then pass it its Entity
                // Client may have to do predictive entity creation in the future,
                // but we'll avoid that here for now because client has to wait anyways
                clientInfo.entity = create_client_focal_entity(cosmos, m_sharedBroker);
                PLEEPLOG_DEBUG("Created entity " + std::to_string(clientInfo.entity) + " for client " + std::to_string(msg.remote->get_id()));

                // Forward new client message to signal for cosmos to initialize client side entities
                PLEEPLOG_DEBUG("Sending new client acknowledgement to new client");
                msg.msg.header.coherency = currentCoherency;
                msg.msg << clientInfo;
                msg.remote->send(msg.msg);

                // TODO: we should also keep track of the clientInfo.entity
                // (store it in msg.remote?)
                // when we receive input updates from this client, we should only allow
                // and/or assume they are for this entity only!
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(msg.remote->get_id()) + "] Received unknown message");
            }
            break;
            }
        }

        // Fourth: Process all submitted data in relays

        // Fifth: After all ingesting is done, send/broadcast fresh data to clients
        if (cosmos)
        {
            Signature downstreamSign = cosmos->get_category_signature(ComponentCategory::downstream);
            for (auto signIt : cosmos->get_signatures_ref())
            {
                EventMessage updateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params updateInfo = {
                    signIt.first,
                    signIt.second & downstreamSign
                };
                cosmos->serialize_entity_components(updateInfo.entity, updateInfo.sign, updateMsg);
                updateMsg << updateInfo;
                m_networkApi.broadcast_message(updateMsg);


                // DON'T broadcast superposition entities into the past
                // TODO: better way to determine superposition without exceptions?
                try
                {
                    bool superposition = cosmos->get_component<TransformComponent>(updateInfo.entity).superposition;
                    if (superposition) continue;
                }
                catch(...)
                {
                    // This entity didn't have a TransformComponent, therefore it cannot be in a superposition
                }

                // 5b: Broadcast to past timestreams 
                //     with incremented CausalChainLink
                if (m_timelineApi.has_past())
                {
                    updateMsg >> updateInfo;
                    increment_causal_chain_link(updateInfo.entity);
                    updateMsg << updateInfo;

                    m_timelineApi.push_past_timestream(updateInfo.entity, updateMsg);
                }
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
        // WARN: when entity is created it has no components, so it cannot be updated until ENTITY_MODIFIED events
        m_networkApi.broadcast_message(entityEvent);

        events::cosmos::ENTITY_CREATED_params newEntityParams;
        entityEvent >> newEntityParams;
        
        //PLEEPLOG_DEBUG("Handling ENTITY_CREATED event for entity " + std::to_string(newEntityParams.entity));

        // Entity cannot be created in a superposition?

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
    
    void ServerNetworkDynamo::_entity_modified_handler(EventMessage entityEvent)
    {
        //PLEEPLOG_DEBUG("Handling ENTITY_MODIFIED event");
        
        m_networkApi.broadcast_message(entityEvent);

        // entity signature has been modified
        events::cosmos::ENTITY_MODIFIED_params modEntityParams;
        entityEvent >> modEntityParams;
        
        // TODO: check superposition
        if (m_timelineApi.has_past())
        {
            events::cosmos::ENTITY_MODIFIED_params propogateModEntityParams = modEntityParams;
            increment_causal_chain_link(propogateModEntityParams.entity);
            entityEvent << propogateModEntityParams;

            m_timelineApi.push_past_timestream(propogateModEntityParams.entity, entityEvent);
        }
    }
    
    void ServerNetworkDynamo::_entity_removed_handler(EventMessage entityEvent)
    {
        //PLEEPLOG_DEBUG("Handling ENTITY_REMOVED event");

        m_networkApi.broadcast_message(entityEvent);

        events::cosmos::ENTITY_REMOVED_params removedEntityParams;
        entityEvent >> removedEntityParams;
        entityEvent << removedEntityParams; // re-fill message for forwarding

        // entity has left our timeslice so we need to decrement its host's count
        TimesliceId host = derive_timeslice_id(removedEntityParams.entity);
        m_timelineApi.send_message(host, entityEvent);

        // TODO: check superposition
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
    
    void ServerNetworkDynamo::_intercession_handler(EventMessage intercessionEvent)
    {
        PLEEPLOG_DEBUG("Handling INTERCESSION event");

        events::network::INTERCESSION_params intercessionInfo;
        intercessionEvent >> intercessionInfo;
        intercessionEvent << intercessionInfo;

        // signal to future timeslice to put intercesee into superposition
        // maybe the future timeslice should clear the timestream to avoid race condition?
        //   this means response time may be delayed by 1 frame? Theoretically more... it is not strictly guarenteed that another timeslice will process a frame before we process another one...

        PLEEPLOG_INFO("Detected interaction event from entity " + std::to_string(intercessionInfo.intercessor) + " to " + std::to_string(intercessionInfo.intercessee));

        // We also need to be able to detect knock-on interactions an object with a non-zero chainlink might cause after being modified by a chainlink zero entity, before and after superposition resolution
        // how is superposition represented? Static bool array in Cosmos? Maybe enum: normal, superposition, superposition-start

        if (m_timelineApi.has_future())
        {
            // timesliceId decreases into the future
            assert(m_timelineApi.get_timeslice_id() > 0);
            m_timelineApi.send_message(m_timelineApi.get_timeslice_id() - 1, intercessionEvent);
        }
    }
}