#include "server_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"
#include "staging/cosmos_builder.h"
#include "staging/client_focal_entity.h"
#include "staging/jump_vfx.h"
#include "core/i_cosmos_context.h"

namespace pleep
{
    // number of frames a forking state has lasted where it becomes a candidate for resolution
    constexpr uint16_t FORKING_THRESHOLD = static_cast<uint16_t>(0.3 * pleep::FRAMERATE);
    // number of frames a forked state has lasted which triggers a resolution
    constexpr uint16_t FORKED_THRESHOLD = static_cast<uint16_t>(0.3 * pleep::FRAMERATE);
    // FORKING_THRESHOLD + FORKED_THREASHOLD == total time until an interception triggers resolution

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
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_STATE_CHANGE, ServerNetworkDynamo::_timestream_state_change_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ServerNetworkDynamo::_jump_request_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::JUMP_ARRIVAL, ServerNetworkDynamo::_jump_arrival_handler));

        PLEEPLOG_TRACE("Done server networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        // clear handlers
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ServerNetworkDynamo::_entity_created_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ServerNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ServerNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_STATE_CHANGE, ServerNetworkDynamo::_timestream_state_change_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ServerNetworkDynamo::_jump_request_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::JUMP_ARRIVAL, ServerNetworkDynamo::_jump_arrival_handler));
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
        {
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
                    PLEEPLOG_WARN("Received ENTITY_CREATED signal for an entity we don't host. Is this intentional?");
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
            case  events::cosmos::ENTITY_UPDATE:
            {
                // NOT a notification, this is a direct state override called by another server
                // for safety we should only allow these on an entity in superposition
                // (to at least guarentee it was a previously expected update)


                // removing superposition should re-enable past-ward propagation

            }
            case events::network::COHERENCY_SYNC:
            {
                events::network::COHERENCY_SYNC_params data;
                msg >> data;
                uint16_t targetCoherency = msg.header.coherency - (m_timelineApi.get_timeslice_delay()*FRAMERATE);

                if (targetCoherency != cosmos->get_coherency())
                {
                    PLEEPLOG_DEBUG("Detected Coherency Desync! COHERENCY_SYNC notification from slice " + std::to_string(data.senderId) + " of " + std::to_string(targetCoherency) + " does not match my coherency " + std::to_string(cosmos->get_coherency()));

                    cosmos->set_coherency(targetCoherency);
                }
            }
            break;
            case events::cosmos::TIMESTREAM_STATE_CHANGE:
            {
                // we are signalled that an interception has occurred sometime/where from another timeslice

                events::cosmos::TIMESTREAM_STATE_CHANGE_params data;
                msg >> data;
                PLEEPLOG_TRACE("Received TIMESTREAM_STATE_CHANGE event for entity " + std::to_string(data.entity) + " to state " + std::to_string(data.newState));
                
                TimesliceId host = derive_timeslice_id(data.entity);
                GenesisId genesis = derive_genesis_id(data.entity);
                CausalChainlink link = derive_causal_chain_link(data.entity);

                switch (data.newState)
                {
                    case TimestreamState::forking:
                    case TimestreamState::forked:
                    {
                        // entity of same link should match (coming from parallel)
                        if (cosmos->entity_exists(data.entity))
                        {
                            cosmos->set_timestream_state(data.entity, TimestreamState::forked);
                        }

                        // any entity with a lesser chainlink should be set to superposition
                        // (state change of a link-0 entity doesn't have any effect)
                        for (CausalChainlink pLink = link - 1U; pLink < NULL_CAUSALCHAINLINK; pLink--)
                        {
                            Entity testEntity = compose_entity(host, genesis, pLink);

                            if (cosmos->entity_exists(testEntity))
                            {
                                cosmos->set_timestream_state(testEntity, TimestreamState::superposition);
                            }
                        }
                    }
                    break;
                    default:
                    {

                    }
                    break;
                }
            }
            break;
            case events::network::JUMP_REQUEST:
            {
                PLEEPLOG_TRACE("Received jump request from a server");
                
                /// TODO: Receive JUMP_REQUEST from server (validate) then overwrite it into a JUMP_ARRIVAL (keeping serialized data intact)
                /// emit JUMP_ARRIVAL on event broker, register entity locally (push arrival to past) and then unpack arrival
                /// past should then see a creation, and then an unpackable arrival, and then updates
                /// Back here (after event) we send a JUMP_ARRIVAL notification back to source (implying an arrival has occurred elsewhere in spacetime)
                /// source then pushes a DEPARTURE (sends same to repective client) and then condemns the entity

                // convert message into an arrival for response (maintaining serialised data):
                msg.header.id = events::network::JUMP_ARRIVAL;

                events::network::JUMP_params jumpInfo;
                msg >> jumpInfo;

                int source = m_timelineApi.get_timeslice_id() - jumpInfo.timesliceDelta;
                // inverse delta to be relative to our local time
                jumpInfo.timesliceDelta *= -1;

                if (m_clientEntities.count(jumpInfo.entity) > 0 || cosmos->entity_exists(jumpInfo.entity))
                {
                    PLEEPLOG_ERROR("Entity already exists at destination during jump request?");
                    // return failure response
                    jumpInfo.port = 0;
                    msg << jumpInfo;

                    // return to sender (arrival error)
                    m_timelineApi.send_message(static_cast<TimesliceId>(source), msg);
                }
                else
                {
                    // pc will need to know where to go
                    jumpInfo.port = m_timelineApi.get_port();
                    msg << jumpInfo;

                    // Agnostic to pc or npc we'll create the entity now, 
                    // and expect pcs will have their clients connect later with tripId
                    cosmos->register_entity(jumpInfo.entity, jumpInfo.sign);
                    // A normal timestream invoked register does NOT increment host count
                    // because it is already done upon pushing that creation event to the timestream
                    // but in this case we need to specially invoke host count increment
                    // (this is in addition to host increment in event handler if we have a past)
                    EventMessage createMsg(events::cosmos::ENTITY_CREATED);
                    events::cosmos::ENTITY_CREATED_params createInfo{
                        jumpInfo.entity,
                        jumpInfo.sign
                    };
                    createMsg << createInfo;
                    m_timelineApi.send_message(derive_timeslice_id(createInfo.entity), createMsg);

                    // handle arrivals generated here or via timestream in same handler:
                    m_sharedBroker->send_event(msg);
                    
                    // save tripId in client map in preparation for client to connect soon
                    if (jumpInfo.isFocal)
                    {
                        // Between this point and client connection
                        // we may accidentally try to access that connection id which is not real
                        // maybe we want a dedicated "transfer" buffer to store waiting trips?
                        m_transferCache.insert({ jumpInfo.tripId, jumpInfo.entity });
                    }

                    // return to sender (arrival success)
                    m_timelineApi.send_message(static_cast<TimesliceId>(source), msg);
                    
                    // Jump Complete!
                }
            }
            break;
            case events::network::JUMP_ARRIVAL:
            {
                PLEEPLOG_DEBUG("Received jump arrival notification from a server");
                // implying an entity has arrived on that server

                events::network::JUMP_params jumpInfo;
                msg >> jumpInfo;
                msg << jumpInfo;

                // if entity belongs to a client forward message to them
                if (jumpInfo.isFocal)
                {
                    uint32_t connId = m_clientEntities.at(jumpInfo.entity);
                    PLEEPLOG_DEBUG("Forwarding it to " + std::to_string(connId));
                    m_networkApi.send_message(connId, msg);
                }

                // check if this is an error arrival (it should be sent to client either way)
                if (jumpInfo.port == 0U)
                {
                    break;
                }
                
                // How do we ensure that host count does not reach 0 prematurely?
                // We must only delete jumping entity now that they are guarenteed to exist elsewhere
                cosmos->condemn_entity(jumpInfo.entity);

                // report departure now (we know arrival was successful)
                if (m_timelineApi.has_past())
                {
                    // convert msg to DEPARTURE
                    msg.header.id = events::network::JUMP_DEPARTURE;
                    // increment into past
                    msg >> jumpInfo;
                    increment_causal_chain_link(jumpInfo.entity);
                    msg << jumpInfo;
                    msg.header.coherency = cosmos->get_coherency();
                    // MUST still contain serialized data from initial JUMP_REQUEST!
                    m_timelineApi.push_past_timestream(jumpInfo.entity, msg);
                }

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
            case events::parallel::INIT:
            {
                PLEEPLOG_DEBUG("Received init request from parallel context");
                // parallel is ready to start and has requested we init it
                m_timelineApi.parallel_load_and_link(cosmos);

                // check if load was successful
                if (m_timelineApi.parallel_get_timeslice() != m_timelineApi.get_timeslice_id())
                {
                    // someone else beat us, parallel is not mirroring us...
                    PLEEPLOG_WARN("Someone: " + std::to_string(m_timelineApi.parallel_get_timeslice()) + " beat us to initing... this doesn't seem right.");
                    break;
                }

                // estimate a lower bound of where the next slice could be (to avoid overshooting)
                m_timelineApi.parallel_retarget(cosmos->get_coherency() + m_timelineApi.get_timeslice_delay()*FRAMERATE - 1U);
                m_timelineApi.parallel_start();

                // future server will continue to update target and handle FINISHED...
            }
            break;
            case events::parallel::FINISHED:
            {
                PLEEPLOG_DEBUG("Received finished notification from parallel context at coherency " + std::to_string(msg.header.coherency) + " on my coherency " + std::to_string(cosmos->get_coherency()));

                // check if coherency matches
                if (cosmos->get_coherency() == msg.header.coherency)
                {
                    // sync sucessful
                    m_timelineApi.parallel_extract(cosmos);
                }
                else if (coherency_greater_or_equal(cosmos->get_coherency(), msg.header.coherency))
                {
                    // my coherency is ahead of parallel:
                    // restart it and try to get it to match on next frame
                    m_timelineApi.parallel_retarget(cosmos->get_coherency() + 1U);
                    m_timelineApi.parallel_start();
                }
                else
                {
                    // my coherency is behind parallel:
                    // uhh... we can't store messages for next frame...
                    // try restarting and getting it to send another event?
                    PLEEPLOG_ERROR("Parallel finished ahead of local cosmos... stalling to next frame...");
                    m_timelineApi.parallel_start();
                }
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Received unknown message id " + std::to_string(msg.header.id));
            }
            }
        }
        }

        // Second: handle incoming timestream messages from future timeslice
        // This should get our cosmos as close as possible to realtime
        //_process_timestream_messages();
        if (m_timelineApi.has_future() && cosmos != nullptr)
        {
            // we want to iterate through all entries in the future EntityTimestreamMap
            // not necessarily our cosmos' entities...
            std::vector<Entity> availableEntities = m_timelineApi.get_entities_with_future_streams();

            for (Entity& evntEntity : availableEntities)
            {
                EventMessage evnt(1);
                // timeline api will return false if nothing available at currentCoherency or earlier
                while (m_timelineApi.pop_future_timestream(evntEntity, currentCoherency, evnt))
                {
                    // pop will stop if next message is too far in future, but
                    // if message is from too far in the past ignore it also
                    if (evnt.header.coherency < cosmos->get_coherency()) continue;
                    
                    // Handle timestream messages just as a client would, i think?
                    switch(evnt.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        evnt >> updateInfo;
                        assert(updateInfo.entity == evntEntity);
                        //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());

                        // ensure entity exists
                        if (!cosmos->entity_exists(updateInfo.entity))
                        {
                            PLEEPLOG_ERROR("Received ENTITY_UPDATE for entity " + std::to_string(updateInfo.entity) + " which does not exist, skipping...");
                            break;
                        }

                        /// if this is divergent entity, then only read upstream components
                        if (is_divergent(cosmos->get_timestream_state(updateInfo.entity).first))
                        {
                            Signature upstreamSign = cosmos->get_category_signature(ComponentCategory::upstream);
                            
                            for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
                            {
                                if (!updateInfo.sign.test(i))
                                {
                                    continue;
                                }
                                if (upstreamSign.test(i))
                                {
                                    cosmos->deserialize_single_component(updateInfo.entity, i, evnt);
                                }
                                else
                                {
                                    cosmos->discard_single_component(i, evnt);
                                }
                            }
                        }
                        else
                        {
                            // if non-divergent entity, then read update into Cosmos as normal
                            cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, evnt, updateInfo.category);
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        // register entity and create components to match signature
                        events::cosmos::ENTITY_CREATED_params createInfo;
                        evnt >> createInfo;
                        assert(createInfo.entity == evntEntity);
                        PLEEPLOG_DEBUG("Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                        cosmos->register_entity(createInfo.entity, createInfo.sign);
                    }
                    break;
                    case events::cosmos::ENTITY_REMOVED:
                    {
                        // call cosmos to delete entity, it will emit event again over broker and cause host decrement
                        // (sender SHOULD have correctly offset causalchainlink already)
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        evnt >> removeInfo;
                        assert(removeInfo.entity == evntEntity);
                        PLEEPLOG_TRACE("Remove Entity: " + std::to_string(removeInfo.entity));

                        // use condemn event to avoid double deletion
                        cosmos->condemn_entity(removeInfo.entity);
                    }
                    break;
                    case events::network::JUMP_REQUEST:
                    {
                        // Just ignore this and wait for the subsiquent departure (if jump was successful)
                    }
                    break;
                    case events::network::JUMP_DEPARTURE:
                    {
                        // we should expect a condemn to occur later this frame.
                        // need to emit this on event broker? maybe later.

                        events::network::JUMP_params jumpInfo;
                        evnt >> jumpInfo;

                        //assert(jumpInfo.entity == evntEntity);
                        if (jumpInfo.entity != evntEntity)
                        {
                            PLEEPLOG_CRITICAL("Jump departure for entity " + std::to_string(jumpInfo.entity) + " on stream for entity: " + std::to_string(evntEntity));
                            break;
                        }

                        // forward this departure into the timestream with the same tripId
                        // but relative to our present entity
                        if (m_timelineApi.has_past())
                        {
                            increment_causal_chain_link(jumpInfo.entity);
                            // timesliceDelta remains same

                            evnt << jumpInfo;
                            m_timelineApi.push_past_timestream(jumpInfo.entity, evnt);
                        }
                    }
                    break;
                    case events::network::JUMP_ARRIVAL:
                    {
                        // handle arrivals in timestream or directly after JUMP_REQUEST the same:
                        m_sharedBroker->send_event(evnt);
                    }
                    break;
                    default:
                    {
                        // reached end of this entities timestream
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
                if (cosmos) cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, remoteMsg.msg, ComponentCategory::upstream);
            }
            break;
            case events::network::NEW_CLIENT:
            {
                // this is the first message a client will send us after connection
                PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received new client notice");

                events::network::NEW_CLIENT_params clientInfo;
                remoteMsg.msg >> clientInfo;
                
                // if we have no cosmos at this point we can't proceed
                // maybe we should deny client connections if there is no cosmos?
                // assert(cosmos);
                if (!cosmos) break;
                
                PLEEPLOG_WARN("New client joined, I should send them a cosmos config!");
                remoteMsg.remote->enable_sending();

                /// TODO: Send (Intercession) app info
                // num timeslices, current timeslice, cosmos configuration?
                EventMessage appMsg(events::network::APP_INFO);
                events::network::APP_INFO_params appInfo = get_app_info();
                appMsg << appInfo;
                remoteMsg.remote->send(appMsg);

                // Cosmos config
                // should be stateless and scan current workingCosmos?
                // How should scan work, just store config into Cosmos when it is built? What if it changes afterwards?
                // If CosmosBuilder could use synchro/component typeid then we could scan Cosmos' registries
                // CosmosBuilder::Config needs to be serializable... are std::sets serializable?

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
                    events::cosmos::ENTITY_CREATED_params createInfo = {
                        signIt.first,
                        signIt.second & cosmos->get_category_signature(ComponentCategory::downstream)
                    };
                    createMsg << createInfo;
                    //PLEEPLOG_DEBUG("Sending message: " + createMsg.info());
                    remoteMsg.remote->send(createMsg);
                }

                
                // check transfer codes for available entity
                if (clientInfo.transferCode != 0 && m_transferCache.count(clientInfo.transferCode))
                {
                    // entity should be ready and waiting already
                    clientInfo.entity = m_transferCache.at(clientInfo.transferCode);
                    m_transferCache.erase(clientInfo.transferCode);
                }
                else
                {
                    // Server will create client character and then pass it its Entity
                    // Client may have to do predictive entity creation in the future,
                    // but we'll avoid that here for now because client has to wait anyways
                    glm::vec3 spawnPoint = glm::vec3(0.0f, 3.0f, 0.0f);

                    clientInfo.entity = create_client_focal_entity(cosmos, spawnPoint);
                    PLEEPLOG_DEBUG("Created new entity " + std::to_string(clientInfo.entity) + " for client " + std::to_string(remoteMsg.remote->get_id()));
                    // "jump" into the simulation
                    create_jump_vfx(cosmos, clientInfo.entity, spawnPoint);
                }

                // send client upstream signture update for its focal entity
                EventMessage updateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params updateInfo = {
                    clientInfo.entity,
                    cosmos->get_entity_signature(clientInfo.entity) & cosmos->get_category_signature(ComponentCategory::upstream),
                    ComponentCategory::upstream
                };
                cosmos->serialize_entity_components(updateInfo.entity, updateInfo.sign, updateMsg);

                updateMsg << updateInfo;
                remoteMsg.remote->send(updateMsg);

                // Forward new client message to signal for cosmos to initialize client side entities
                PLEEPLOG_DEBUG("Sending new client acknowledgement to new client");
                remoteMsg.msg.header.coherency = currentCoherency;
                remoteMsg.msg << clientInfo;
                remoteMsg.remote->send(remoteMsg.msg);

                // we should also keep track of the clientInfo.entity ourselves
                // when we receive input updates from this client, we can validate the target entity
                // and ensure they only affect this entity only!
                assert(m_clientEntities.count(clientInfo.entity) == 0);
                m_clientEntities.insert({ clientInfo.entity, remoteMsg.remote->get_id() });
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remoteMsg.remote->get_id()) + "] Received unknown message");
            }
            }
        }


        // Fourth: After all ingesting is done, send/broadcast fresh downstream data to clients & children
        if (cosmos)
        {
            for (auto signIt : cosmos->get_signatures_ref())
            {
                // push to clients
                EventMessage clientUpdateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params clientUpdateInfo = {
                    signIt.first,
                    signIt.second & cosmos->get_category_signature(ComponentCategory::downstream),
                    ComponentCategory::downstream
                };
                cosmos->serialize_entity_components(clientUpdateInfo.entity, clientUpdateInfo.sign, clientUpdateMsg);

                clientUpdateMsg << clientUpdateInfo;
                m_networkApi.broadcast_message(clientUpdateMsg);


                // and push to child timestream (except if there is not past to push to)
                if (!m_timelineApi.has_past())
                {
                    continue;
                }

                EventMessage childUpdateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params childUpdateInfo = {
                    signIt.first,
                    signIt.second,  // full signature of components
                    ComponentCategory::all
                };
                cosmos->serialize_entity_components(childUpdateInfo.entity, childUpdateInfo.sign, childUpdateMsg);

                // increment chain link (increase) when moving into the past
                increment_causal_chain_link(childUpdateInfo.entity);
                childUpdateMsg << childUpdateInfo;
                m_timelineApi.push_past_timestream(childUpdateInfo.entity, childUpdateMsg);
            }
        }

        // Fifth: Check and update timestream states, and update parallel cosmos as appropriate
        if (cosmos)
        {
            // TODO: short circuit if parallel is already simulating our past


            bool resolutionNeeded = false;
            for (auto signIt : cosmos->get_signatures_ref())
            {
                const std::pair<TimestreamState, uint16_t> state = cosmos->get_timestream_state(signIt.first);
                // promote forking entities
                if (state.first == TimestreamState::forking
                    && state.second + FORKING_THRESHOLD <= cosmos->get_coherency())
                {
                    cosmos->set_timestream_state(signIt.first, TimestreamState::forked);
                }

                if (state.first == TimestreamState::forked
                    && state.second + FORKED_THRESHOLD <= cosmos->get_coherency())
                {
                    //PLEEPLOG_DEBUG("Entity " + std::to_string(signIt.first) + " is due for resolution at: " + std::to_string(cosmos->get_coherency()));
                    resolutionNeeded = true;
                }
            }
            if (resolutionNeeded) m_timelineApi.parallel_notify_divergence();

            /// If parallel is simulating our recent past, we need to continually feed it new target coherencies (m_lastCoherency + 1) until FINISHED event is sent, and it moves onto the next
            /// We never want to restart the thread, only let FINSIHED handler try to restart.
            if (m_timelineApi.parallel_get_timeslice() == cosmos->get_host_id() + 1U)
            {
                m_timelineApi.parallel_retarget(cosmos->get_coherency() + 1);
            }
        }
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        // clear our working cosmos
        m_workingCosmos.reset();
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
    
    events::network::APP_INFO_params ServerNetworkDynamo::get_app_info()
    {
        events::network::APP_INFO_params appInfo;
        appInfo.currentTimeslice = m_timelineApi.get_timeslice_id();
        appInfo.totalTimeslices = m_timelineApi.get_num_timeslices();
        return appInfo;
    }
    
    void ServerNetworkDynamo::_entity_created_handler(EventMessage creationEvent)
    {   
        // Broadcast creation event to clients, the run_relays update will populate it
        m_networkApi.broadcast_message(creationEvent);

        events::cosmos::ENTITY_CREATED_params newEntityParams;
        creationEvent >> newEntityParams;
        
        //PLEEPLOG_DEBUG("Handling ENTITY_CREATED event for entity " + std::to_string(newEntityParams.entity));

        // Entity cannot be created in a non-normal timestreamState?

        // if entity was created locally, its host count starts at 1
        // if entity has propagated from the future, it will have been incrememnted when it entered our future timestream

        // If we have a past timeslice, that means next frame this entity updates will enter the past timestream
        if (m_timelineApi.has_past())
        {
            events::cosmos::ENTITY_CREATED_params propogateNewEntityParams = newEntityParams;
            increment_causal_chain_link(propogateNewEntityParams.entity);
            creationEvent << propogateNewEntityParams;

            m_timelineApi.push_past_timestream(propogateNewEntityParams.entity, creationEvent);

            // signal for host to increment now that entity is in the past
            TimesliceId host = derive_timeslice_id(propogateNewEntityParams.entity);
            m_timelineApi.send_message(host, creationEvent);
        }
    }
    
    void ServerNetworkDynamo::_entity_removed_handler(EventMessage removalEvent)
    {
        //PLEEPLOG_DEBUG("Handling ENTITY_REMOVED event");
        // REMEMBER: this is called AFTER the entity was destroyed so you can't lookup any of its data

        m_networkApi.broadcast_message(removalEvent);

        events::cosmos::ENTITY_REMOVED_params removedEntityParams;
        removalEvent >> removedEntityParams;
        removalEvent << removedEntityParams; // re-fill message for forwarding

        // entity has left our timeslice so we need to decrement its host's count
        TimesliceId host = derive_timeslice_id(removedEntityParams.entity);
        // Don't send to ourselves, decrement will already have happened
        if (host != m_timelineApi.get_timeslice_id()) m_timelineApi.send_message(host, removalEvent);

        // if it was a client's focal entity, remove it from client map
        m_clientEntities.erase(removedEntityParams.entity);

        // propagate further down the timeline
        if (m_timelineApi.has_past())
        {
            events::cosmos::ENTITY_REMOVED_params propogateRemovedEntityParams;
            removalEvent >> propogateRemovedEntityParams;
            increment_causal_chain_link(propogateRemovedEntityParams.entity);
            removalEvent << propogateRemovedEntityParams;

            m_timelineApi.push_past_timestream(propogateRemovedEntityParams.entity, removalEvent);

            // once child receives the above event, it will signal another decrement
        }
    }
    
    void ServerNetworkDynamo::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        // Something (like collision) has detected an interception between two entities
        PLEEPLOG_TRACE("Handling TIMESTREAM_INTERCEPTION event");

        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;
        interceptionEvent << interceptionInfo;

        CausalChainlink link = derive_causal_chain_link(interceptionInfo.recipient);
        if (link <= 0)
        {
            PLEEPLOG_ERROR("Something went wrong, a link-0 entity (" + std::to_string(interceptionInfo.recipient) + ") cannot have been interrupted. Ignoring...");
            return;
        }
        
        PLEEPLOG_TRACE("Detected interaction event from entity " + std::to_string(interceptionInfo.agent) + " to " + std::to_string(interceptionInfo.recipient));

        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        // Put the entity into a forking timestream state (stop receiving from future)
        cosmos->set_timestream_state(interceptionInfo.recipient, TimestreamState::forking);
        // "Divergent" state (forking) now restricts reading downstream components from the timestream
    }

    void ServerNetworkDynamo::_timestream_state_change_handler(EventMessage stateEvent)
    {
        // entity state has changed in our local cosmos
        events::cosmos::TIMESTREAM_STATE_CHANGE_params stateInfo;
        stateEvent >> stateInfo;
        stateEvent << stateInfo;
        //PLEEPLOG_TRACE("Handling TIMESTREAM_STATE_CHANGE event for entity " + std::to_string(stateInfo.entity) + " to state " + std::to_string(stateInfo.newState));

        // what states do we want clients to know about?
        // forking/superposition: inform server
        // merged/superposition: inform clients
        switch (stateInfo.newState)
        {
        case TimestreamState::merged:
        {
            m_networkApi.broadcast_message(stateEvent);
        }
        break;
        case TimestreamState::superposition:
        {
            // signal to both client
            m_networkApi.broadcast_message(stateEvent);
            // and other timeslices
        }
        // fallthrough:
        case TimestreamState::forking:
        {
            // signal to ALL timeslices, who check for any temporalentity with a lesser chainlink
            m_timelineApi.broadcast_message(stateEvent);
        }
        break;
        default:
        {
            // ignore any other state changes
        }
        }
    }

    void ServerNetworkDynamo::_jump_request_handler(EventMessage jumpEvent)
    {
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;
        assert(jumpEvent.header.id == events::network::JUMP_REQUEST);

        // some behavior has called for an entity to jump
        // Message should not yet have serialized data!
        events::network::JUMP_params jumpInfo;
        jumpEvent >> jumpInfo;

        // requests triggered by propogated entities (chainlink > 0) are duplicates and can be ignored
        if (derive_causal_chain_link(jumpInfo.entity) > 0) return;

        // generate our own tripId (can't trust client)
        uint32_t timeSeed = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
        );
        jumpInfo.tripId = Squirrel3(jumpInfo.entity, timeSeed);

        // forward request to destination timeslice based on timesliceDelta
        int destination = m_timelineApi.get_timeslice_id() + jumpInfo.timesliceDelta;

        // check if this entity belongs to a client right now
        if (m_clientEntities.count(jumpInfo.entity) > 0)
        {
            jumpInfo.isFocal = true;
        }

        // ensure destination is in range AND is not same as present
        if (destination < 0 || destination > m_timelineApi.get_num_timeslices() - 1 || destination == m_timelineApi.get_timeslice_id())
        {
            /// TODO: if client invoked this jump inform then that request is invalid?

            return;
        }

        // build message data (now that we're server side)
        jumpInfo.sign = cosmos->get_entity_signature(jumpInfo.entity);
        cosmos->serialize_entity_components(jumpInfo.entity, jumpInfo.sign, jumpEvent);
        jumpEvent << jumpInfo;
        
        // good to send request now
        m_timelineApi.send_message(static_cast<TimesliceId>(destination), jumpEvent);

        // request is successful so we'll spawn vfx (departure may not happen but thats fine)
        if (cosmos->has_component<TransformComponent>(jumpInfo.entity))
        {
            create_jump_vfx(cosmos, jumpInfo.entity, cosmos->get_component<TransformComponent>(jumpInfo.entity).origin);
        }

        // push request to timestream so that it can be validated during parallel
        if (m_timelineApi.has_past())
        {
            jumpEvent >> jumpInfo;
            increment_causal_chain_link(jumpInfo.entity);

            jumpEvent << jumpInfo;
            jumpEvent.header.coherency = cosmos->get_coherency();
            m_timelineApi.push_past_timestream(jumpInfo.entity, jumpEvent);
        }
    }

    void ServerNetworkDynamo::_jump_arrival_handler(EventMessage jumpEvent)
    {
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        // arrival has happened on the timestream (or directly after a JUMP_REQUEST)
        // entity should have been created already, so just unpack

        events::network::JUMP_params jumpInfo;
        jumpEvent >> jumpInfo;

        // Push jump arrival to timeline (AFTER creation event/before unpacking)
        if (m_timelineApi.has_past())
        {
            increment_causal_chain_link(jumpInfo.entity);
            // timesliceDelta remains same

            jumpEvent << jumpInfo;
            jumpEvent.header.coherency = cosmos->get_coherency();
            m_timelineApi.push_past_timestream(jumpInfo.entity, jumpEvent);

            // restore message/info state for use in present
            jumpEvent >> jumpInfo;
            decrement_causal_chain_link(jumpInfo.entity);
        }

        assert(cosmos->entity_exists(jumpInfo.entity));
        // entity exists, write components to it
        cosmos->deserialize_entity_components(jumpInfo.entity, jumpInfo.sign, jumpEvent, ComponentCategory::all);

        
        // request is successful so we'll spawn vfx
        if (cosmos->has_component<TransformComponent>(jumpInfo.entity))
        {
            create_jump_vfx(cosmos, jumpInfo.entity, cosmos->get_component<TransformComponent>(jumpInfo.entity).origin);
        }
    }
}