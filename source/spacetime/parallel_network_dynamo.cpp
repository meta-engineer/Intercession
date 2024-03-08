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

        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::JUMP_DEPARTURE, ParallelNetworkDynamo::_jump_departure_handler));
        
        PLEEPLOG_TRACE("Done parallel networking pipeline setup");
    }
    
    ParallelNetworkDynamo::~ParallelNetworkDynamo() 
    {
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelNetworkDynamo::_timestream_interception_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::INIT, ParallelNetworkDynamo::_parallel_init_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::parallel::FINISHED, ParallelNetworkDynamo::_parallel_finished_handler));
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::JUMP_DEPARTURE, ParallelNetworkDynamo::_jump_departure_handler));
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
        if (m_timelineApi.has_future())
        {
            std::vector<Entity> availableEntities = m_timelineApi.get_entities_with_future_streams();

            for (Entity& entity : availableEntities)
            {
                EventMessage evnt(1);
                // timeline api will return false if nothing available at currentCoherency or earlier
                while (m_timelineApi.pop_future_timestream(entity, currentCoherency, evnt))
                {
                    // continue to clear out messages if no working cosmos
                    if (!cosmos) continue;

                    switch(evnt.header.id)
                    {
                    case events::cosmos::ENTITY_UPDATE:
                    {
                        events::cosmos::ENTITY_UPDATE_params updateInfo;
                        evnt >> updateInfo;

                        if (!cosmos->entity_exists(updateInfo.entity))
                        {
                            break;
                        }

                        // ONLY EXTRACT UPSTREAM COMPONENTS!
                        cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, evnt, ComponentCategory::upstream);
                    }
                    break;
                    case events::cosmos::ENTITY_CREATED:
                    {
                        // this could be an entity created after jumping
                        // same as server?
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
                        // repeat removal
                        events::cosmos::ENTITY_REMOVED_params removeInfo;
                        evnt >> removeInfo;
                        PLEEPLOG_TRACE("Remove Entity: " + std::to_string(removeInfo.entity));

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

                        events::network::JUMP_DEPARTURE_params jumpInfo;
                        evnt >> jumpInfo;
                        PLEEPLOG_WARN("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ PARALLEL TIMESTREAM DEPARTURE FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));

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
                        events::network::JUMP_ARRIVAL_params jumpInfo;
                        evnt >> jumpInfo;
                        PLEEPLOG_WARN("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& PARALLEL TIMESTREAM ARRIVAL FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));

                        // if we have a matching divergentJump, then update with the cached data
                        if (m_divergentDepartures.count(jumpInfo.tripId))
                        {
                            PLEEPLOG_INFO("Overwriting arrival state with divergent departure data");
                            // This entity should already have been created before arrival event
                            assert(cosmos->entity_exists(jumpInfo.entity));

                            events::network::JUMP_DEPARTURE_params cachedJumpInfo;
                            m_divergentDepartures.at(jumpInfo.tripId) >> cachedJumpInfo;

                            cosmos->deserialize_entity_components(jumpInfo.entity, cachedJumpInfo.sign, m_divergentDepartures.at(jumpInfo.tripId));

                            // we've extracted everything, so pop this message.
                            m_divergentDepartures.erase(jumpInfo.tripId);

                            // we don't want this to immediately get updated with the timestream
                            // so lets consider this forked
                            cosmos->set_timestream_state(jumpInfo.entity, TimestreamState::forked);
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

    void ParallelNetworkDynamo::_jump_departure_handler(EventMessage jumpEvent)
    {
        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.lock();
        if (m_workingCosmos.expired()) return;

        // our own cosmos has triggered a jump departure
        // check our cached network departures earlier this frame?
        // But if the entity jumps and is deleted during network step... then how will the jump
        //   trigger interally? If it is only condemned after jumping, then we still have
        //   1 frame for it to trigger? Needs testing
        // REMEMBER this is just the "request" it has not been serialized

        events::network::JUMP_DEPARTURE_params jumpInfo;
        jumpEvent >> jumpInfo;

        PLEEPLOG_WARN("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PARALLEL COSMOS DEPARTURE FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.timesliceDelta));

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

            /// TODO: how to indicate another cycle?
            /// need to samehow call ParallelCosmosContext::request_resolution
            /// probably need another dedicated event to signal divergence that needs to be resolved
            EventMessage divMessage(events::parallel::DIVERGENCE);
            // ensure this triggers a re-cycle
            events::parallel::DIVERGENCE_params divInfo{ NULL_TIMESLICEID };
            divMessage << divInfo;
            m_sharedBroker->send_event(divMessage);
        }
        
        // remove unmatched pair
        m_departureConditions.erase(jumpInfo.entity);
    }
}
