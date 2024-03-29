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

                        if (!cosmos->entity_exists(updateInfo.entity)) break;

                        /// When a paradox occurs we may want the entity to be forced to follow its state in the timestream instead of only following the upstream
                        /// So perhaps we want to specifically carry over its "forked" state from the previous worldline, even though it doesn't interact with any forked entity
                        /// Then we can set it to merged when we know it is causing a paradox
                        /// Thus, if something new comes along, it will re-fork it and it will behave inline with upstream only... (possibly causing another paradox)

                        // ONLY EXTRACT UPSTREAM COMPONENTS!
                        if (is_divergent(cosmos->get_timestream_state(updateInfo.entity).first))
                        {
                            cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, evnt, ComponentCategory::upstream);
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
                    case events::network::JUMP_REQUEST:
                    {
                        // A jump is about to happen on the timestream, 
                        // we should soon see a local cosmos jump request
                        // BEFORE the departure with the matching tripID

                        events::network::JUMP_params jumpInfo;
                        evnt >> jumpInfo;
                        evnt << jumpInfo;
                        PLEEPLOG_WARN("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ PARALLEL TIMESTREAM REQUEST FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));
                        assert(jumpInfo.entity == entity);

                        // DO NOT forward this to past, wait for local request to do that (if ever)
                        
                        // store jump conditions for matching with local request later
                        /// TODO: check entry already exists somehow?
                        assert(m_jumpConditions.count(jumpInfo.entity) < 1);
                        m_jumpConditions.insert({ jumpInfo.entity, extract_jump_conditions(evnt, cosmos) });

                        // clear any previously stored request divergences for this tripId
                        // we'll generate a new one (if applicable) when we handle the cosmos' request
                        m_divergentJumpRequests.erase(jumpInfo.tripId);
                    }
                    break;
                    case events::network::JUMP_DEPARTURE:
                    {
                        // entity jumped here in the current history
                        // we need to determine if the events of history have diverged "significantly" enough
                        // and the departure which occurs in-cosmos does not match this one
                        // meaning the arrival with the corresponding tripId no longer makes sense
                        // (including if this departure NEVER occurs in-cosmos)

                        // Networker happens before behaver this frame, so we should store this event,
                        // and wait for the behaver to trigger its version.
                        // Do we have any margin for being off frame?
                        // departures are emplaced upon first event emission

                        events::network::JUMP_params jumpInfo;
                        evnt >> jumpInfo;
                        evnt << jumpInfo;
                        assert(jumpInfo.entity == entity);
                        PLEEPLOG_WARN("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ PARALLEL TIMESTREAM DEPARTURE FOR: " + std::to_string(jumpInfo.entity) + " | " + std::to_string(jumpInfo.tripId));

                        // if we've reached the departure, then clear the request's conditions
                        m_jumpConditions.erase(jumpInfo.entity);
                        /// TODO: can we be robust to receiving the departure too early?

                        // nothing to do without past, just ignore it
                        if (!m_timelineApi.has_past()) break;

                        // propogate a departure event into the past depending on...
                        // if we have a divergent departure for this trip use that
                        // if this departure is nulled then ignore it
                        // if departure is not divergent use this as-is

                        if (m_divergentJumpRequests.count(jumpInfo.tripId))
                        {
                            // convert to departure, and increment for past
                            EventMessage& newJump = m_divergentJumpRequests.at(jumpInfo.tripId);
                            newJump.header.id = events::network::JUMP_DEPARTURE;
                            newJump.header.coherency = evnt.header.coherency;

                            events::network::JUMP_params newJumpInfo;
                            newJump >> newJumpInfo;
                            newJump << newJumpInfo;

                            // ensure this trip matches our chainlink (it could be "old", leftover from future slice)
                            if (newJumpInfo.entity == jumpInfo.entity)
                            {
                                PLEEPLOG_DEBUG("Using cached divergent departure: " + std::to_string(newJumpInfo.tripId) + " for entity " + std::to_string(newJumpInfo.entity));

                                m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, newJump);
                                break;
                            }
                        }

                        // if no divergent jump, or jump was too old:
                        PLEEPLOG_DEBUG("No divergent departure to use");
                        m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, evnt);
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
                        if (m_divergentJumpRequests.count(jumpInfo.tripId))
                        {
                            PLEEPLOG_INFO("Overwriting arrival state with divergent departure data");

                            EventMessage& cachedJump = m_divergentJumpRequests.at(jumpInfo.tripId);
                            // push THIS version of arrival to past
                            if (m_timelineApi.has_past())
                            {
                                cachedJump.header.id = events::network::JUMP_ARRIVAL;
                                cachedJump.header.coherency = evnt.header.coherency;
                                // parallel is in same timeframe as past, so no increment chain link
                                m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, cachedJump);
                            }

                            events::network::JUMP_params cachedJumpInfo;
                            cachedJump >> cachedJumpInfo;
                            assert(jumpInfo.entity == cachedJumpInfo.entity);

                            cosmos->deserialize_entity_components(jumpInfo.entity, cachedJumpInfo.sign, cachedJump);

                            // we've extracted everything, so pop the cache
                            m_divergentJumpRequests.erase(jumpInfo.tripId);
                            
                            // we don't want this to immediately get updated with the timestream
                            // so lets consider this forked
                            cosmos->set_timestream_state(jumpInfo.entity, TimestreamState::forked);
                        }
                        // if not then then forward it into history as-is, and use arrival data directly
                        else
                        {
                            PLEEPLOG_INFO("Arrival is normal, forwarding as-is");
                            if (m_timelineApi.has_past())
                            {
                                // parallel is in same timeframe as past, so no increment chain link
                                evnt << jumpInfo;

                                m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, evnt);

                                // restore evnt as it was before
                                evnt >> jumpInfo;
                            }

                            cosmos->deserialize_entity_components(jumpInfo.entity, jumpInfo.sign, evnt);
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
        m_jumpConditions.clear();

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

        /// Assuming that the destination server will always send SOME response (either arrival failure, or arrival success) then at that point we can enact our decision (or assume that the jump would fail just as well)

        events::network::JUMP_params jumpInfo;
        jumpEvent >> jumpInfo;

        PLEEPLOG_WARN("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PARALLEL COSMOS JUMP REQUEST FOR: " + std::to_string(jumpInfo.entity) + ", delta: " + std::to_string(jumpInfo.timesliceDelta));

        // manually extract components from cosmos, not the message:
        TimejumpConditions newConditions{ 0 };
        if (cosmos->has_component<TransformComponent>(jumpInfo.entity))
        {
            newConditions.origin = cosmos->get_component<TransformComponent>(jumpInfo.entity).origin;
        }

        // complete jump request (like server does)
        jumpInfo.sign = cosmos->get_entity_signature(jumpInfo.entity);
        cosmos->serialize_entity_components(jumpInfo.entity, jumpInfo.sign, jumpEvent);
        //jumpEvent << jumpInfo;

        // Check if newly simulated jump matches cached jump
        if (m_jumpConditions.count(jumpInfo.entity) < 1)
        {
            // This means a jump was triggered which didn't happen in the original history?
            PLEEPLOG_DEBUG("?????????????????????????????????????????? MISSING PARALLEL TIMESTREAM REQUEST");
            /// TODO: We could store this somehow, extrapolate the coherency it should arrive, and emplace it into the timeline next cycle?
            /// TODO: generate new tripId?
            jumpInfo.tripId = 91339;
            jumpEvent << jumpInfo;
        }
        else if (newConditions == m_jumpConditions[jumpInfo.entity])
        {
            // jump is same, history has not changed
            PLEEPLOG_DEBUG(".......................................... EQUIVALENT JUMP REQUEST DETECTED: " + std::to_string(m_jumpConditions[jumpInfo.entity].tripId));
            PLEEPLOG_DEBUG("Comparing new: " + std::to_string(newConditions.origin.x) + ", " + std::to_string(newConditions.origin.y) + ", " + std::to_string(newConditions.origin.z) + " to expected: " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.x) + ", " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.y) + ", " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.z));

            /// TODO: Can the departure happen the same frame? is it possible that the other server responds fast enough for this server to handle its request & response in the same queue?
            /// if so... It will already have been handled...

            /// assuming not, when it arrives we should just forward it through as-is?
            // use tripId from original timestream jump?
            jumpInfo.tripId = m_jumpConditions[jumpInfo.entity].tripId;
            jumpEvent << jumpInfo;
        }
        else
        {
            // jump is in a divergent history
            PLEEPLOG_DEBUG("########################################## DIVERGENT JUMP REQUEST DETECTED: " + std::to_string(m_jumpConditions[jumpInfo.entity].tripId));
            PLEEPLOG_DEBUG("Comparing new: " + std::to_string(newConditions.origin.x) + ", " + std::to_string(newConditions.origin.y) + ", " + std::to_string(newConditions.origin.z) + " to expected: " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.x) + ", " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.y) + ", " + std::to_string(m_jumpConditions[jumpInfo.entity].origin.z));

            // store request for when we encounter the arrival with matching tripId
            // use tripId from original timestream jump?
            jumpInfo.tripId = m_jumpConditions[jumpInfo.entity].tripId;
            jumpEvent << jumpInfo;
            PLEEPLOG_DEBUG("Storing new for use at matching arrival.");
            m_divergentJumpRequests.insert({ jumpInfo.tripId, jumpEvent });

            /// indicate another parallel cycle is needed
            /// TODO: this should depend on if jump is to past or future
            EventMessage divMessage(events::parallel::DIVERGENCE);
            // null id should ensure this triggers a re-cycle
            // technically this should be the "simulated" timeslice in cosmos context...
            events::parallel::DIVERGENCE_params divInfo{ NULL_TIMESLICEID };
            divMessage << divInfo;
            m_sharedBroker->send_event(divMessage);
        }

        // remove cached previous history
        m_jumpConditions.erase(jumpInfo.entity);

        // forward request event to past (whether it is diverged, or not diverged and same as previous)
        if (m_timelineApi.has_past())
        {
            jumpEvent.header.coherency = cosmos->get_coherency();
            // DO NOT increment if pushing at breakpoint!
            m_timelineApi.push_timestream_at_breakpoint(jumpInfo.entity, jumpEvent);
        }
    }
}
