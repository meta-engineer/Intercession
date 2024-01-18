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
    
    bool ParallelCosmosContext::load_and_link(const std::shared_ptr<Cosmos> sourceCosmos, const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams)
    {
        PLEEPLOG_DEBUG("Loading parallel from timeslice " + std::to_string(sourceCosmos->get_host_id()));
        // not running does not guarentee simulation is ready (extraction is complete)
        // but it prevents the thread blowing up at least
        if (is_running()) return false;

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
        // build new timestreams
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
        // - remove SpacetimeComponent of all entities which aren't forked

        PLEEPLOG_DEBUG("Copying over entities");

        // CosmosConfig should setup all synchros, dynamos, and eventbroker
        // Then we can simply copy every entity over through serialization
        for (auto signMapIt : sourceCosmos->get_signatures_ref())
        {
            // omit anything that doesn't exist in the future at all (time travellers)
            if (!sourceFutureTimestreams->entity_has_timestream(signMapIt.first))
            {
                PLEEPLOG_DEBUG("Skipping entity " + std::to_string(signMapIt.first)+ " because it has no future");
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
                false,
                entityUpdate
            );
            assert(m_currentCosmos->entity_exists(signMapIt.first));

            // after initing parallel cosmos all entity timestream state should be re-merged in local
            // (as we are about to resolve them)
            // (decided to also do this for things forkING)
            // AND we should clear their timestreams in source
            if (sourceCosmos->has_component<SpacetimeComponent>(signMapIt.first)
                && is_divergent(sourceCosmos->get_component<SpacetimeComponent>(signMapIt.first).timestreamState))
            {
                sourceCosmos->remove_component<SpacetimeComponent>(signMapIt.first);
                sourceFutureTimestreams->clear(signMapIt.first);
            }
        }

        m_currentTimeslice = sourceCosmos->get_host_id();
        
        return true;
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
    
    bool ParallelCosmosContext::extract_entity_updates(std::shared_ptr<Cosmos> dstCosmos)
    {
        PLEEPLOG_DEBUG("Extracting parallel to timeslice " + std::to_string(dstCosmos->get_host_id()));

        const std::lock_guard<std::mutex> lk(m_accessMux);

        // ensure cosmos exists!?
        if (m_currentCosmos == nullptr)
        {
            PLEEPLOG_WARN("Called extract while Cosmos does not exist?! Something went wrong!");
            return false;
        }

        // ensure cosmos is stopped (it should already be)
        if (is_running())
        {
            PLEEPLOG_WARN("Called extract while Cosmos is still running");
            return false;
        }

        // coherency values must be same or syncronization has messed up
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
            if (!decrement_causal_chain_link(localEntity))
            {
                // ignore non-temporal entities
                // (or entities with chainlink 0, how did they even get forked to begin with?)
                continue;
            }

            // extract if entity is forked OR is NULLTIMESLICEID
            // if it is NULL_TIMESLICEID then create new id
            if (derive_timeslice_id(signMapIt.first) == NULL_TIMESLICEID)
            {
                /// TODO: created entities to be extracted to destination (with new id)
                /// use parallel entity as source for local entity
                continue;
            }
            else if (m_currentCosmos->has_component<SpacetimeComponent>(signMapIt.first))
            {
                /// local but forked entities to be extracted to destination (including forked state)
                m_currentCosmos->serialize_entity_components(signMapIt.first, signMapIt.second, extraction);

                // entity should already exist
                dstCosmos->deserialize_entity_components(localEntity, signMapIt.second, false, extraction);
            }

            // if this is timeslice 0 then clear all forked entity states NOW (usually happens during next init, but timeslice 0 will deflect init call)
            if (dstCosmos->get_host_id() == 0U &&
                dstCosmos->has_component<SpacetimeComponent>(localEntity))
            {
                dstCosmos->remove_component<SpacetimeComponent>(localEntity);
            }
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
        // otherwise send init request to timesliceId - 1 until we reach timeslice 0
        else if (dstCosmos->get_host_id() > 0U)
        {
            EventMessage initMessage(events::parallel::INIT);
            events::parallel::INIT_params initInfo{ dstCosmos->get_host_id() - 1U };
            initMessage << initInfo;
            m_eventBroker->send_event(initMessage);
        }
        // otherwise we're done until later requests

        /// TODO: Entities created partway through a simulation won't properly propogate into the past
        /// We may have to keep a parallel past timestream, and splice it into local past timestream when extracting...
        /// Then the previous timeslice will have a "seamless" timestream

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
        if (m_currentCosmos->get_coherency() >= m_coherencyTarget)
        {
            PLEEPLOG_DEBUG("Parallel reached coherency target of " + std::to_string(m_currentCosmos->get_coherency()));

            // coherency is updated AFTER all fixed step relays,
            // so we want to exit before any fixed steps this cycle

            // don't run this frame
            m_timeRemaining = std::chrono::duration<double>(-2147483647);
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
        m_timeRemaining = m_fixedTimeStep;

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