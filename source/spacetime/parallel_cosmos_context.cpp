#include "parallel_cosmos_context.h"

#include "staging/hard_config_cosmos.h"
#include "spacetime/parallel_network_dynamo.h"

namespace pleep
{
    ParallelCosmosContext::ParallelCosmosContext(TimelineApi localTimelineApi)
        : I_CosmosContext()
        , m_timelineApi(localTimelineApi)
    {
        // dynamos should only be for headless simulation
        m_dynamoCluster.networker = std::make_shared<ParallelNetworkDynamo>(m_eventBroker, localTimelineApi);
        m_dynamoCluster.behaver   = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);

        // event handlers
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelCosmosContext::_entity_removed_handler));
    }

    ParallelCosmosContext::~ParallelCosmosContext()
    {
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelCosmosContext::_entity_removed_handler));
    }

    void ParallelCosmosContext::request_resolution(TimesliceId requesterId)
    {
        PLEEPLOG_DEBUG("Timeslice " + std::to_string(requesterId) + " is requesting resolution");
        const std::lock_guard<std::mutex> lk(m_accessMux);

        // if already simulating, add request to special buffer?
        // if not simulating send init request to requester
        // simulating != running
        // use cosmos existing as indication???
        if (m_currentTimeslice == NULL_TIMESLICEID)
        {
            // send init request back to requesterId via event to network dynamo
            EventMessage initMessage(events::parallel::INIT);
            events::parallel::INIT_params initInfo{ requesterId };
            initMessage << initInfo;
            m_eventBroker->send_event(initMessage);
        }
        else
        {
            // if we haven't reached requesterId then no worries, we'll get there this cycle
            // if we are at or behind requesterId then we need another cycle
            if (m_currentTimeslice > requesterId)
            {
                /// set dirty bit to be checked at the end of a cycle, so that we start another cycle to resolve it
                m_resolutionNeeded = true;
            }
        }
    }
    
    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        const std::lock_guard<std::mutex> lk(m_accessMux);
        
        m_coherencyTarget = coherency;
    }
    
    TimesliceId ParallelCosmosContext::get_current_timeslice()
    {
        const std::lock_guard<std::mutex> lk(m_accessMux);

        return m_currentTimeslice;
    }
    
    bool ParallelCosmosContext::load_and_link(const std::shared_ptr<Cosmos> sourceCosmos, const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams)
    {
        PLEEPLOG_DEBUG("Loading parallel from timeslice " + std::to_string(sourceCosmos->get_host_id()));
        // not running does not guarentee simulation is ready (extraction is complete)
        // but it prevents the thread blowing up at least
        if (is_running()) return false;

        assert(sourceCosmos != nullptr);
        assert(sourceFutureTimestreams != nullptr);

        const std::lock_guard<std::mutex> lk(m_accessMux);

        // if we were already init by someone else first then skip:
        if (m_currentTimeslice != NULL_TIMESLICEID) return false;

        // keep parallel's ID as NULL_TIMESLICEID, and register local entities as foreign
        // then convert NULL_TIMESLICEID entities to local upon extraction

        // build new cosmos
        PLEEPLOG_DEBUG("Initializing cosmos");
        /// TEMP: use hard-coded cosmos config
        m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);
        m_currentCosmos->set_coherency(sourceCosmos->get_coherency());
        PLEEPLOG_DEBUG("Setting cosmos to start at coherency " + std::to_string(m_currentCosmos->get_coherency()));

        PLEEPLOG_DEBUG("Copying over timestreams");
        // mirror and link timestreams together to maintain sync after load
        m_timelineApi.link_future_timestreams(sourceFutureTimestreams);


        // I can't just copy cosmos as it is because any time travellers aren't in the future...
        // I need to copy the cosmos as it is in the timestream, 
        // EXCEPT the resolution candidates need to be how they are in the cosmos directly...
        // so that excludes any forkING entities, and time-travellers

        // copy the cosmos as it is now, BUT:
        // - delete/omit any entities which do not have a future timestream (time-travellers)
        // - for any forking entities, use their timestream states instead?
        //      - Can we run out of timestream, can it not be available after a recent interception?
        //      - maybe... just use them as-is, but don't extract them, so they stay the same, but affect the simulation correctly 
        // - remove timestreamstate of all entities which aren't forked

        PLEEPLOG_DEBUG("Copying over entities");

        // CosmosConfig should setup all synchros, dynamos, and eventbroker
        // Then we can simply copy every entity over through serialization
        for (auto signMapIt : sourceCosmos->get_signatures_ref())
        {
            // omit anything that doesn't exist in the future at all (time travellers)
            if (!sourceFutureTimestreams->entity_has_timestream(signMapIt.first))
            {
                PLEEPLOG_DEBUG("Skipping entity " + std::to_string(signMapIt.first) + " because it has no future");
                continue;
            }

            EventMessage entityUpdate(events::cosmos::ENTITY_UPDATE);
            sourceCosmos->serialize_entity_components(
                signMapIt.first,
                signMapIt.second,
                entityUpdate
            );

            PLEEPLOG_DEBUG("Creating parallel entity " + std::to_string(signMapIt.first));
            if (!m_currentCosmos->register_entity(signMapIt.first))
            {
                PLEEPLOG_DEBUG("Entity registration failed?");
                continue;
            }
            PLEEPLOG_DEBUG("Entity registration suceeded");
            m_currentCosmos->deserialize_entity_components(
                signMapIt.first,
                signMapIt.second,
                entityUpdate,
                ComponentCategory::all
            );
            assert(m_currentCosmos->entity_exists(signMapIt.first));

            // after transferring components timestream state should be transferred as well
            // meaning setting forked entities in parallel, and re-merging them in source
            // (as we are about to resolve them)
            // should this be forkING entities as well? kinda defeats the point of having another state
            if (sourceCosmos->get_timestream_state(signMapIt.first).first == TimestreamState::forked)
            {
                m_currentCosmos->set_timestream_state(signMapIt.first, TimestreamState::forked);
                sourceCosmos->set_timestream_state(signMapIt.first, TimestreamState::merging);
            }
        }

        m_currentTimeslice = sourceCosmos->get_host_id();
        
        return true;
    }
 
    bool ParallelCosmosContext::extract_entity_updates(std::shared_ptr<Cosmos> dstCosmos)
    {
        PLEEPLOG_DEBUG("Extracting parallel to timeslice " + std::to_string(dstCosmos->get_host_id()));

        const std::lock_guard<std::mutex> lk(m_accessMux);

        // ensure cosmos exists!?
        assert(m_currentCosmos != nullptr);
        if (m_currentCosmos == nullptr)
        {
            PLEEPLOG_WARN("Called extract while Cosmos does not exist?! Something went wrong!");
            return false;
        }

        // ensure cosmos is stopped (it should already be)
        assert(!is_running());
        if (is_running())
        {
            PLEEPLOG_WARN("Called extract while Cosmos is still running");
            return false;
        }

        // coherency values must be same or syncronization has messed up
        assert(m_currentCosmos->get_coherency() == dstCosmos->get_coherency());
        if (m_currentCosmos->get_coherency() != dstCosmos->get_coherency())
        {
            PLEEPLOG_ERROR("Called extract while Cosmos' are not synced, something went wrong. Ignoring...");
            return false;
        }

        // add/update existing entities
        for (auto signMapIt : m_currentCosmos->get_signatures_ref())
        {
            EventMessage extraction(events::cosmos::ENTITY_UPDATE);
            
            Entity localEntity = signMapIt.first;
            assert(derive_causal_chain_link(localEntity) != 0);
            if (!decrement_causal_chain_link(localEntity))
            {
                // ignore non-temporal entities
                // (or entities with chainlink 0, how did they even get forked to begin with?)
                PLEEPLOG_ERROR("Failed to extract entity " + std::to_string(localEntity) + " because its causal chain link value is: " + std::to_string(derive_causal_chain_link(localEntity)));
                continue;
            }

            // create any new entities (with NULL_TIMESLICEID)
            // and extract any entity local entities which have diverged during simulation
            if (derive_timeslice_id(localEntity) == NULL_TIMESLICEID)
            {
                /// TODO: locally created entities to be extracted to destination with new id
                // (once we have the ability to create entities at all)
                /// use parallel entity as source for local entity
                continue;
            }
            // easier to extract ALL local entities, than to compute non-divergences somehow
            else if (true)//(is_divergent(m_currentCosmos->get_timestream_state(signMapIt.first).first))
            {
                m_currentCosmos->serialize_entity_components(signMapIt.first, signMapIt.second, extraction);

                // entity should already exist
                if (!dstCosmos->entity_exists(localEntity))
                {
                    PLEEPLOG_ERROR("Non-null-host entity " + std::to_string(localEntity) + " from past does not already exist in future.");
                }
                assert(dstCosmos->entity_exists(localEntity));

                /// parallel forked entities to be extracted to destination
                dstCosmos->deserialize_entity_components(localEntity, signMapIt.second, extraction, ComponentCategory::all);
                
                // carry over their forked state to ensure copying to next parallel
                dstCosmos->set_timestream_state(localEntity, m_currentCosmos->get_timestream_state(signMapIt.first).first);

                PLEEPLOG_TRACE("Extracted entity: " + std::to_string(signMapIt.first));
            }
            else
            {
                // entity was not created nor forked during parallel
                PLEEPLOG_TRACE("Ignoring entity: " + std::to_string(signMapIt.first));
            }

            // if this is timeslice 0 then clear all forked entity states NOW
            // (this usually happens during state transfer in next init, but timeslice 0 will deflect init call)
            if (dstCosmos->get_host_id() == 0U)
            {
                dstCosmos->set_timestream_state(localEntity, TimestreamState::merged);
            }

            // overwrite this entities recent past with parallels history?
            // alternatively if we could set all current timestream events to upstream only
            //   it would continue simulating (theoretically) identically to parallel's history
            // an then every NEW timestream event from here onward would be a full update...

            // we could... stuff a re-merge event into timestream now, so entity stays mergING until it reaches this point...
            // it should theoretically follow the same deterministic path as it did while forked inside parallel

            // will this short-cut cause problems during loop unrolling?
            // Since it depends on timestream state being merging to avoid reading the outdated data, if state is not maintained it will follow the old history.

            // Clear mergING state for this entity in the recent past:
            //m_timelineApi.send_message(m_currentTimeslice, ...);
        }

        // remove any local entities from the destination which were condemned during the run
        for (auto dead : m_condemnedEntities)
        {
            if (!decrement_causal_chain_link(dead))
            {
                // ignore entities which could not exist anyways
                continue;
            }
            if (derive_timeslice_id(dead) != NULL_TIMESLICEID)
            {
                dstCosmos->condemn_entity(dead);
            }
        }
        // condemned entities won't exist upon next init
        m_condemnedEntities.clear();


        // EXTRACTION COMPLETE!
        m_currentCosmos = nullptr;
        m_currentTimeslice = NULL_TIMESLICEID;
        m_timelineApi.unlink_future_timestreams();

        // if this is timeslice 0 AND m_resolutionNeeded is true,
        //   then send new init request to past-most timeslice
        if (dstCosmos->get_host_id() == 0U && m_resolutionNeeded)
        {
            EventMessage initMessage(events::parallel::INIT);
            events::parallel::INIT_params initInfo{ static_cast<uint16_t>(m_timelineApi.get_num_timeslices() - 1U) };
            initMessage << initInfo;
            m_eventBroker->send_event(initMessage);
            m_resolutionNeeded = false;
        }
        // otherwise send init request to same timesliceId until we reach timeslice 0
        else if (dstCosmos->get_host_id() > 0U)
        {
            EventMessage initMessage(events::parallel::INIT);
            events::parallel::INIT_params initInfo{ dstCosmos->get_host_id() };
            initMessage << initInfo;
            m_eventBroker->send_event(initMessage);
        }
        // otherwise we're done until later requests

        /// TODO: Entities created partway through a simulation won't properly propogate into the past
        /// We may have to keep a parallel past timestream, and splice it into local past timestream when extracting...
        ///   then the previous timeslice will have a "seamless" timestream
        /// ALSO promote "merging" entities in the past to "merged" once the timestream is spliced
        ///   the forked set in parallel cosmos is a superset of the merging set in the past, so we could send event to past to update all to promote to merged...

        return true;
    }

    void ParallelCosmosContext::_entity_removed_handler(EventMessage removalEvent)
    {
        // This one is probably hard to avoid...
        // deletion of any non-null host ids will have to be stored
        // and deleted in local during extraction...

        events::cosmos::ENTITY_REMOVED_params removalInfo;
        removalEvent >> removalInfo;

        m_condemnedEntities.insert(removalInfo.entity);
    }


    void ParallelCosmosContext::_prime_frame() 
    {
        if (!m_currentCosmos) return;

        // check if we reached our target before running next step
        if (coherency_greater_or_equal(m_currentCosmos->get_coherency(), m_coherencyTarget))
        {
            PLEEPLOG_DEBUG("Parallel reached coherency target of " + std::to_string(m_currentCosmos->get_coherency()));

            // coherency is updated AFTER all fixed step relays,
            // so we want to exit before any fixed steps this cycle

            // don't run this frame
            m_fixedTimeRemaining = std::chrono::duration<double>(-2147483647);
            // don't run next frame
            this->stop();

            // signal that target is reached
            EventMessage finishedEvent(events::parallel::FINISHED);
            finishedEvent.header.coherency = m_currentCosmos->get_coherency();
            assert(m_currentTimeslice > 0); // should never be built from slice 0
            events::parallel::FINISHED_params finishedInfo{ m_currentTimeslice - 1U };
            finishedEvent << finishedInfo;
            m_eventBroker->send_event(finishedEvent);

            return;
        }

        // artifically give ample time to reach target without waiting
        // pretend we are always behind in simulation time, and need to catch up
        m_fixedTimeRemaining = m_fixedTimeStep;

        m_currentCosmos->update();
    }
    
    void ParallelCosmosContext::_on_fixed(double fixedTime) 
    {
        m_dynamoCluster.networker->run_relays(fixedTime);
        m_dynamoCluster.behaver->run_relays(fixedTime);
        m_dynamoCluster.physicser->run_relays(fixedTime);
    }
    
    void ParallelCosmosContext::_on_frame(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void ParallelCosmosContext::_clean_frame() 
    {
        // flush dynamos of all synchro submissions
        m_dynamoCluster.networker->reset_relays();
        m_dynamoCluster.behaver->reset_relays();
        m_dynamoCluster.physicser->reset_relays();
    }
}