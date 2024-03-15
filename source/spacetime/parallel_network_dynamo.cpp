#include "parallel_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"
#include "networking/pleep_crypto.h"

namespace pleep
{
    ParallelNetworkDynamo::ParallelNetworkDynamo(std::shared_ptr<EventBroker> sharedBroker, TimelineApi localTimelineApi) 
        : I_NetworkDynamo(sharedBroker)
        , m_timelineApi(localTimelineApi)
    {
        PLEEPLOG_TRACE("Start parallel networking pipeline setup");

        // handle entity created/removed/intercepted for extraction later?
        // Do not send ENTITY_CREATED/REMOVED events to entity's host timeslice

        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ParallelNetworkDynamo::_entity_created_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ParallelNetworkDynamo::_jump_request_handler));
        
        PLEEPLOG_TRACE("Done parallel networking pipeline setup");
    }
    
    ParallelNetworkDynamo::~ParallelNetworkDynamo() 
    {
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ParallelNetworkDynamo::_entity_created_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelNetworkDynamo::_entity_removed_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ParallelNetworkDynamo::_jump_request_handler));
    }
    
    void ParallelNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) cosmos = nullptr;
        
        uint16_t currentCoherency = 0;
        if (cosmos) currentCoherency = cosmos->get_coherency();
        
        /// handle incoming timestream messages from linked future timeslice
        //_process_timestream_messages();
        if (m_timelineApi.has_future() && cosmos != nullptr)
        {
            std::vector<Entity> availableEntities = m_timelineApi.get_entities_with_future_streams();

            for (Entity& entity : availableEntities)
            {
                EventMessage evnt(1);
                // timeline api will return false if nothing available at currentCoherency or earlier
                while (m_timelineApi.pop_timestream_at_breakpoint(entity, currentCoherency, evnt))
                {
                    switch(evnt.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        evnt >> updateInfo;
                        //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());
                        assert(updateInfo.entity == entity);

                        if (!cosmos->entity_exists(updateInfo.entity))
                        {
                            break;
                        }

                        // FOR FORKED ONLY EXTRACT UPSTREAM COMPONENTS!
                        if (cosmos->get_timestream_state(updateInfo.entity).first == TimestreamState::merged)
                        {
                            cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, evnt, ComponentCategory::all);
                        }
                        else
                        {
                            cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, evnt, ComponentCategory::upstream);
                        }
                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        // this could be an entity created after jumping
                        // same as server?
                        events::cosmos::ENTITY_CREATED_params createInfo;
                        evnt >> createInfo;
                        assert(createInfo.entity == entity);
                        PLEEPLOG_DEBUG("Parallel Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                        cosmos->register_entity(createInfo.entity, createInfo.sign);
                    }
                    break;
                    case events::cosmos::ENTITY_REMOVED:
                    {
                        // repeat removal
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        evnt >> removeInfo;
                        assert(removeInfo.entity == entity);
                        PLEEPLOG_TRACE("Parallel Remove Entity: " + std::to_string(removeInfo.entity));

                        // use condemn event to avoid double deletion
                        cosmos->condemn_entity(removeInfo.entity);
                    }
                    break;
                    case events::network::JUMP_DEPARTURE:
                    {
                        // encountered a jump during re-simulation
                        // we need to determine if the events of the re-sim have diverged "significantly" enough
                        // and the departure which occurs in-cosmos does not match this one
                        // meaning the arrival with the corresponding tripId no longer makes sense
                        // (including if this departure NEVER occurs in-cosmos)

                        // Networker happens before behaver this frame, so we should store this event,
                        // and wait for the behaver to trigger its version.
                        // Do we have any margin for being off frame?
                        // departures are emplaced upon first event emission

                        events::network::JUMP_params jumpInfo;
                        evnt >> jumpInfo;
                        assert(jumpInfo.entity == entity);
                        PLEEPLOG_WARN("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ PARALLEL TIMESTREAM DEPARTURE FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));

                        // don't yet know if this is the correct departure state, it could have diverged

                        // store jump tripId for matching with arrival later
                        TimejumpConditions jumpConditions { jumpInfo.tripId };
                        // manually extract components:
                        const ComponentType transformType = cosmos->get_component_type<TransformComponent>();
                        for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
                        {
                            if (!jumpInfo.sign.test(i))
                            {
                                continue;
                            }
                            if (transformType == i)
                            {
                                TransformComponent jumperTransform;
                                evnt >> jumperTransform;
                                jumpConditions.origin = jumperTransform.origin;
                            }
                            else
                            {
                                cosmos->discard_single_component(i, evnt);
                            }
                        }

                        /// TODO: check entry already exists somehow?
                        m_departureConditions.insert({ jumpInfo.entity, jumpConditions });
                    }
                    break;
                    case events::network::JUMP_ARRIVAL:
                    {
                        events::network::JUMP_params jumpInfo;
                        evnt >> jumpInfo;
                        assert(jumpInfo.entity == entity);
                        PLEEPLOG_WARN("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& PARALLEL TIMESTREAM ARRIVAL FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));

                        // creation event should have appeared before this
                        assert(cosmos->entity_exists(jumpInfo.entity));

                        // if arrival's match was divergent then use cached departure & push it to new history
                        if (m_divergentDepartures.count(jumpInfo.tripId))
                        {
                            PLEEPLOG_INFO("Overwriting arrival state with divergent departure data");

                            events::network::JUMP_params cachedJumpInfo;
                            m_divergentDepartures.at(jumpInfo.tripId) >> cachedJumpInfo;
                            
                            if (m_timelineApi.has_past())
                            {
                                // parallel is in same timeframe as past, so no increment chain link
                                m_divergentDepartures.at(jumpInfo.tripId) << cachedJumpInfo;
                                // timesliceDelta remains same

                                m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, evnt);
                                // restore evnt as it was before
                                m_divergentDepartures.at(jumpInfo.tripId) >> cachedJumpInfo;
                            }

                            cosmos->deserialize_entity_components(jumpInfo.entity, cachedJumpInfo.sign, m_divergentDepartures.at(jumpInfo.tripId));

                            // we've extracted everything, so pop this message.
                            m_divergentDepartures.erase(jumpInfo.tripId);
                            
                            // we don't want this to immediately get updated with the timestream
                            // so lets consider this forked
                            cosmos->set_timestream_state(jumpInfo.entity, TimestreamState::forked);
                        }
                        // if not then then forward it into history as-is, and use arrival data directly
                        else
                        {
                            if (m_timelineApi.has_past())
                            {
                                // parallel is in same timeframe as past, so no increment chain link
                                evnt << jumpInfo;
                                // timesliceDelta remains same

                                m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, evnt);
                                // restore evnt as it was before
                                evnt >> jumpInfo;
                            }

                            cosmos->deserialize_entity_components(jumpInfo.entity, jumpInfo.sign, evnt, ComponentCategory::all);
                        }
                    }
                    break;
                    default:
                    {
                        PLEEPLOG_DEBUG("Parallel encountered unknown timestream event: " + std::to_string(evnt.header.id));
                    }
                    }
                }
            }
        }

        // After all ingesting is done, send/broadcast fresh downstream data to past
        if (cosmos && m_timelineApi.has_past())
        {
            for (auto signIt : cosmos->get_signatures_ref())
            {
                EventMessage pastUpdateMsg(events::cosmos::ENTITY_UPDATE, currentCoherency);
                events::cosmos::ENTITY_UPDATE_params pastUpdateInfo = {
                    signIt.first,
                    signIt.second,  // full signature of components
                    ComponentCategory::all
                };
                cosmos->serialize_entity_components(pastUpdateInfo.entity, pastUpdateInfo.sign, pastUpdateMsg);

                // parallel is in same timeframe as past, so no increment chain link
                pastUpdateMsg << pastUpdateInfo;
                m_timelineApi.push_timestream_at_breakpoint(pastUpdateInfo.entity, pastUpdateMsg);
            }
        }
    }
    
    void ParallelNetworkDynamo::reset_relays() 
    {
        m_workingCosmos.reset();

        // we should only check departures which happen in-cosmos and in-timestream on the same frame
        m_departureConditions.clear();

        // any relays?
    }
    
    void ParallelNetworkDynamo::submit(CosmosAccessPacket data) 
    {
        m_workingCosmos = data.owner;
    }

    void ParallelNetworkDynamo::link_timestreams(std::shared_ptr<EntityTimestreamMap> sourceTimestreams)
    {
        m_timelineApi.link_timestreams(sourceTimestreams);
    }

    void ParallelNetworkDynamo::_entity_created_handler(EventMessage creationEvent)
    {
        events::cosmos::ENTITY_CREATED_params creationParams;
        creationEvent >> creationParams;
        
        // forward to past
        if (m_timelineApi.has_past())
        {
            // parallel is in same timeframe as past, so no increment chain link
            creationEvent << creationParams;

            m_timelineApi.push_timestream_at_breakpoint(creationParams.entity, creationEvent);
        }
    }


    void ParallelNetworkDynamo::_entity_removed_handler(EventMessage removalEvent)
    {
        events::cosmos::ENTITY_REMOVED_params removalParams;
        removalEvent >> removalParams;

        // forward to past
        if (m_timelineApi.has_past())
        {
            // parallel is in same timeframe as past, so no increment chain link
            removalEvent << removalParams;

            m_timelineApi.push_timestream_at_breakpoint(removalParams.entity, removalEvent);
        }
    }
    
    void ParallelNetworkDynamo::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;

        // Promote directly to forked (no need to have any forking delay while already in parallel)
        cosmos->set_timestream_state(interceptionInfo.recipient, TimestreamState::forked);
    }
  
    
    void ParallelNetworkDynamo::_parallel_init_handler(EventMessage initEvent)
    {
        events::parallel::INIT_params initInfo;
        initEvent >> initInfo;
        initEvent << initInfo;
        // just forward to source timeslice
        m_timelineApi.send_message(initInfo.sourceTimeslice, initEvent);
    }

    void ParallelNetworkDynamo::_parallel_finished_handler(EventMessage finishedEvent)
    {
        events::parallel::FINISHED_params finishedInfo;
        finishedEvent >> finishedInfo;
        finishedEvent << finishedInfo;
        // just forward to destination timeslice
        m_timelineApi.send_message(finishedInfo.destinationTimeslice, finishedEvent);
    }

    void ParallelNetworkDynamo::_jump_request_handler(EventMessage jumpEvent)
    {
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        // our own cosmos has triggered a jump request
        // Will the timestream or the cosmos request happen first?
        /// TODO: check the timing between request and departure, i think departure is delayed by sending between slices.
        ///   However, we can guarentee that the departure will have the same cahced data as the request because we carry it through the handshake
        ///   and we can guarentee that the entity will not be condemned before it departs
        ///   So we should still be able to match them even if 1-2 frames apart?

        // convert directly to departure
        jumpEvent.header.id = events::network::JUMP_DEPARTURE;

        events::network::JUMP_params jumpInfo;
        jumpEvent >> jumpInfo;

        PLEEPLOG_WARN("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PARALLEL COSMOS JUMP REQUEST FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.timesliceDelta));

        TimejumpConditions newConditions;
        // manually extract components from cosmos:
        if (cosmos->has_component<TransformComponent>(jumpInfo.entity))
        {
            newConditions.origin = cosmos->get_component<TransformComponent>(jumpInfo.entity).origin;
        }

        // Check if newly simulated jump matches cached jump
        if (m_departureConditions.count(jumpInfo.entity) < 1)
        {
            // This means a jump was triggered which didn't happen in the original history?
            PLEEPLOG_DEBUG("Timestream Missing Jump Departure Detected!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#");
            /// TODO: We could store this somehow, extrapolate the coherency it should arrive, and emplace it into the timeline next cycle?
        }
        else if (newConditions == m_departureConditions[jumpInfo.entity])
        {
            // jump is same, history has not changed
        }
        else
        {
            // jump is in a divergent history
            PLEEPLOG_DEBUG("Divergent Jump Departure Detected!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@");

            // serialize the new jump now (as server would do)
            jumpInfo.sign = cosmos->get_entity_signature(jumpInfo.entity);
            cosmos->serialize_entity_components(jumpInfo.entity, jumpInfo.sign, jumpEvent);
            jumpEvent << jumpInfo;

            // store it for when we encounter the arrival with matching tripId
            // use tripId from original timestream jump?
            m_divergentDepartures.insert({ m_departureConditions[jumpInfo.entity].tripId, jumpEvent });
            // restore jumpEvent state
            jumpEvent >> jumpInfo;

            /// TODO: how to indicate another cycle?
            /// need to samehow call ParallelCosmosContext::request_resolution
            /// probably need another dedicated event to signal divergence that needs to be resolved
            EventMessage divMessage(events::parallel::DIVERGENCE);
            // ensure this triggers a re-cycle
            events::parallel::DIVERGENCE_params divInfo{ NULL_TIMESLICEID };
            divMessage << divInfo;
            m_sharedBroker->send_event(divMessage);
        }
        
        // remove cached previous history
        m_departureConditions.erase(jumpInfo.entity);

        // forward event to past (whether it is diverged, or not diverged and same as previous)
        if (m_timelineApi.has_past())
        {
            increment_causal_chain_link(jumpInfo.entity);
            jumpEvent << jumpInfo;
            m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, jumpEvent);
        }
    }
}
