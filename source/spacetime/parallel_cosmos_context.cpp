#include "parallel_cosmos_context.h"

#include <glm/gtc/random.hpp>
#include "staging/hard_config_cosmos.h"
#include "staging/test_projectile.h"
#include "spacetime/parallel_network_dynamo.h"

namespace pleep
{
    ParallelCosmosContext::ParallelCosmosContext(TimelineApi localTimelineApi)
        : I_CosmosContext()
        , m_pastmostTimeslice{ localTimelineApi.get_num_timeslices() > 0 ? localTimelineApi.get_num_timeslices() - 1U : 0U }
    {
        // dynamos should only be for headless simulation
        m_dynamoCluster.networker = std::make_shared<ParallelNetworkDynamo>(m_eventBroker, localTimelineApi);
        m_dynamoCluster.behaver   = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);

        // event handlers
        m_eventBroker->add_listener(METHOD_LISTENER(events::parallel::DIVERGENCE, ParallelCosmosContext::_divergence_handler));
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));
        m_eventBroker->add_listener(METHOD_LISTENER(events::parallel::WORLDLINE_SHIFT, ParallelCosmosContext::_worldline_shift_handler));
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelCosmosContext::_entity_removed_handler));
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ParallelCosmosContext::_entity_created_handler));
    }

    ParallelCosmosContext::~ParallelCosmosContext()
    {
        m_eventBroker->remove_listener(METHOD_LISTENER(events::parallel::DIVERGENCE, ParallelCosmosContext::_divergence_handler));
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));
        m_eventBroker->remove_listener(METHOD_LISTENER(events::parallel::WORLDLINE_SHIFT, ParallelCosmosContext::_worldline_shift_handler));
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_REMOVED, ParallelCosmosContext::_entity_removed_handler));
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::ENTITY_CREATED, ParallelCosmosContext::_entity_created_handler));
    }

    void ParallelCosmosContext::request_resolution(TimesliceId requesterId)
    {
        // no lock needed

        EventMessage divMessage(events::parallel::DIVERGENCE);
        events::parallel::DIVERGENCE_params divInfo{ requesterId };
        divMessage << divInfo;
        m_eventBroker->send_event(divMessage);
    }
    
    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        const std::lock_guard<std::mutex> rLk(m_runtimeMux);
        
        m_coherencyTarget = coherency;
    }
    
    TimesliceId ParallelCosmosContext::get_current_timeslice()
    {
        const std::lock_guard<std::mutex> rLk(m_runtimeMux);

        return m_currentTimeslice;
    }
    
    bool ParallelCosmosContext::load_and_link(const std::shared_ptr<Cosmos> sourceCosmos, const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams)
    {
        assert(sourceCosmos != nullptr);
        assert(sourceFutureTimestreams != nullptr);
        PLEEPLOG_DEBUG("Loading parallel from timeslice " + std::to_string(sourceCosmos->get_host_id()));

        {
            const std::lock_guard<std::mutex> rLk(m_runtimeMux);

            // not running does not guarentee simulation is ready (extraction is complete)
            // but it prevents the thread blowing up at least
            if (is_running()) return false;

            // if we were already init by someone else first then skip:
            if (m_currentState != State::initializing) return false;

            m_currentState = State::busy;

            // release runtime lock
        }

        {
            const std::lock_guard<std::mutex> cLk(m_cosmosMux);

            // keep parallel's ID as NULL_TIMESLICEID, and register local entities as foreign
            // then convert NULL_TIMESLICEID entities to local upon extraction

            // build new cosmos
            //PLEEPLOG_DEBUG("Initializing cosmos");
            /// TEMP: use hard-coded cosmos config
            m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);
            m_currentCosmos->set_coherency(sourceCosmos->get_coherency());
            PLEEPLOG_DEBUG("Setting cosmos to start at coherency " + std::to_string(m_currentCosmos->get_coherency()));

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

                PLEEPLOG_DEBUG("Loading parallel entity " + std::to_string(signMapIt.first) + " | " + signMapIt.second.to_string());

                if (!m_currentCosmos->register_entity(signMapIt.first, signMapIt.second))
                {
                    PLEEPLOG_DEBUG("Entity registration failed?");
                    continue;
                }
                m_currentCosmos->deserialize_entity_components(
                    signMapIt.first,
                    signMapIt.second,
                    entityUpdate,
                    ComponentCategory::all
                );
                assert(m_currentCosmos->entity_exists(signMapIt.first));

                // after transferring components timestream state should be transferred as well
                // meaning setting forked entities in parallel, and re-merge them in source
                // (as we are about to resolve them)
                // should this be forkING entities as well? kinda defeats the point of having another state
                if (sourceCosmos->get_timestream_state(signMapIt.first).first == TimestreamState::forked)
                {
                    m_currentCosmos->set_timestream_state(signMapIt.first, TimestreamState::forked);
                    PLEEPLOG_DEBUG("Setting it as FORKED");
                    sourceCosmos->set_timestream_state(signMapIt.first, TimestreamState::merged);
                }
            }
            
            //PLEEPLOG_DEBUG("Copying over timestreams");
            // do this AFTER entities to avoid passing any init events into timestream
            // link timestreams together to maintain sync after load
            m_dynamoCluster.networker->link_timestreams(sourceFutureTimestreams);
        }

        {
            const std::lock_guard<std::mutex> rLk(m_runtimeMux);

            m_currentState = State::ready;
            m_currentTimeslice = sourceCosmos->get_host_id();

            // release runtime lock
        }
        
        return true;
    }
 
    bool ParallelCosmosContext::extract_entity_updates(std::shared_ptr<Cosmos> dstCosmos)
    {
        PLEEPLOG_DEBUG("Extracting parallel to timeslice " + std::to_string(dstCosmos->get_host_id()));

        {
            const std::lock_guard<std::mutex> rLk(m_runtimeMux);
            
            // ensure cosmos is stopped (it should already be)
            if (is_running()) return false;

            if (m_currentState != State::ready) return false;
            
            // stop anyone from interrupting us.
            m_currentState = State::busy;

            // release runtime lock
        }

        {
            const std::lock_guard<std::mutex> cLk(m_cosmosMux);

            // ensure cosmos exists!?
            if (m_currentCosmos == nullptr)
            {
                PLEEPLOG_WARN("Called extract while Cosmos does not exist?! Something went wrong!");
                assert(m_currentCosmos != nullptr);
            }

            // coherency values must be same or syncronization has messed up
            if (m_currentCosmos->get_coherency() != dstCosmos->get_coherency())
            {
                PLEEPLOG_ERROR("Called extract while Cosmos' are not synced, something went wrong. Ignoring...");
                assert(m_currentCosmos->get_coherency() == dstCosmos->get_coherency());
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
                    PLEEPLOG_ERROR("Ignored extracting entity " + std::to_string(localEntity) + " because its causal chain link value is: " + std::to_string(derive_causal_chain_link(localEntity)));
                    continue;
                }

                // create any new entities (with NULL_TIMESLICEID)
                // and extract any entity local entities which have diverged during simulation
                if (derive_timeslice_id(localEntity) == NULL_TIMESLICEID)
                {
                    /// TODO: locally created entities to be extracted to destination with new id
                    /// TODO: The local entity will also be added to the timestream, which will break the simulation...
                    PLEEPLOG_CRITICAL("Ignored extracting entity " + std::to_string(localEntity) + " because it was created during parallel runtime!!!!");
                    continue;
                }
                else if (m_readingSteinerEntities.count(localEntity))
                {
                    // ignore this entity extraction and keep current timeline's version
                    m_readingSteinerEntities.erase(localEntity);

                    // after decrement this can be a chainlink 0 entity
                    assert(derive_causal_chain_link(localEntity) == 0);

                    // up until this point this entity's timestream will be in parallel's worldline
                    // by ignoring it on extraction this causes it to shift to the physizalized worldline
                    // we need to add an event to the timestream to ensure this discontinuous state change is maintained over cycles

                    // we need to fetch the state of localEntity in dstCosmos and push this into a worldline shift event
                    EventMessage shiftEvent(events::parallel::WORLDLINE_SHIFT, dstCosmos->get_coherency());
                    events::parallel::WORLDLINE_SHIFT_params shiftInfo{ localEntity, dstCosmos->get_entity_signature(localEntity) };
                    dstCosmos->serialize_entity_components(shiftInfo.entity, shiftInfo.sign, shiftEvent);
                    shiftEvent << shiftInfo;
                    // how to access the timestream from here...?
                    m_dynamoCluster.networker->push_to_linked_timestream(shiftEvent, shiftInfo.entity);
                    
                    PLEEPLOG_DEBUG("Worldline shift entity: " + std::to_string(localEntity) + " | " + signMapIt.second.to_string());





                    // create mandella effect for this shift at this moment as well?
                    // worldline shift event happens at arrival time
                    // worldline shift timestream happens here at extraction (shortly after divergence)
                    // create mandella entity directly in dstCosmos? What about other mandella effects?
                    // entity needs to be based on localEntity at the time of the collision?
                    // How do I get information about the readingStienerEntity's divergence event. Collision meta-data?
                    // context can store every interaction event during the cycle
                    // may need more information about interception history (position/velocity at event time?)

                    Entity sourceEntity = signMapIt.first;
                    Entity targetEntity = NULL_ENTITY;

                    // search all history entries from present to past (incrementing) until there is a match
                    {
                        int timesliceDelta = 0;
                        Entity t = sourceEntity;
                        Entity s = NULL_ENTITY;
                        do
                        {
                            if (m_interceptionHistory.count(t) == 0) continue;
                            if (m_interceptionHistory.at(t).empty()) continue;

                            s = m_interceptionHistory.at(t).front();

                            assert(derive_causal_chain_link(s) >= timesliceDelta);
                            for (int i = 0; i < timesliceDelta; i++) decrement_causal_chain_link(s);

                            if (m_currentCosmos->entity_exists(s))
                            {
                                targetEntity = s;
                                break;
                            }
                        } while (increment_causal_chain_link(t) && timesliceDelta++);
                    }

                    if (targetEntity == NULL_ENTITY)
                    {
                        PLEEPLOG_WARN("UH OH! could not find anything in interception history, defaulting to nothing!");
                        targetEntity = sourceEntity;
                    }

                    auto targetTrans = m_currentCosmos->get_component<TransformComponent>(targetEntity);
                    auto sourceTrans = m_currentCosmos->get_component<TransformComponent>(sourceEntity);
                    auto targetPhysics = m_currentCosmos->get_component<PhysicsComponent>(targetEntity);
                    auto sourcePhysics = m_currentCosmos->get_component<PhysicsComponent>(sourceEntity);
                    glm::vec3 diff = sourceTrans.origin - targetTrans.origin;
                    diff = glm::length2(diff) == 0 ? glm::sphericalRand(1.0f) : glm::normalize(diff);

                    // create mandella moving towards source
                    glm::vec3 newVel = (diff * -2.0f) + glm::sphericalRand(1.0f);
                    newVel *= 2.0f;
                    newVel += -targetPhysics.velocity;
                    
                    // TODO: ensure this isn't clipping inside of source? or inside of anything for that matter? raycast from source?
                    // calculate some energy estimate of the collision? create projectile mass/velocity accordingly

                    // velocity may be 0 at this time? derive displacement between both possible entities
                    Entity mandella = create_test_projectile(
                        dstCosmos,
                        sourceEntity, // source should always be forked
                        targetTrans.origin - newVel,
                        newVel
                    );
                    PLEEPLOG_DEBUG("A result of worldline shift created mandella entity: " + std::to_string(mandella));




                    continue;
                }
                // easier to extract ALL local entities, than to compute non-divergences somehow
                else if (true)//(is_divergent(m_currentCosmos->get_timestream_state(signMapIt.first).first))
                {
                    m_currentCosmos->serialize_entity_components(signMapIt.first, signMapIt.second, extraction);

                    // entity should already exist
                    if (!dstCosmos->entity_exists(localEntity))
                    {
                        PLEEPLOG_ERROR("Entity " + std::to_string(localEntity) + " with non-null-host from past does not already exist in future.");
                    }
                    assert(dstCosmos->entity_exists(localEntity));

                    /// parallel forked entities to be extracted to destination
                    dstCosmos->deserialize_entity_components(localEntity, signMapIt.second, extraction, ComponentCategory::all);
                    
                    // carry over their forked state to ensure copying to next parallel
                    if (derive_causal_chain_link(localEntity) > 0) dstCosmos->set_timestream_state(localEntity, m_currentCosmos->get_timestream_state(signMapIt.first).first);

                    PLEEPLOG_DEBUG("Extracted entity: " + std::to_string(localEntity) + " | " + signMapIt.second.to_string());
                }
                else
                {
                    // entity was not created nor forked during parallel
                    PLEEPLOG_DEBUG("Ignoring entity: " + std::to_string(localEntity));
                }

                // if this is timeslice 0 then clear all forked entity states NOW
                // (this usually happens during state transfer in next init, but timeslice 0 will deflect init call)
                if (dstCosmos->get_host_id() == 0U)
                {
                    dstCosmos->set_timestream_state(localEntity, TimestreamState::merged);
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

            // cleaup cosmos
            m_currentCosmos = nullptr;
            // unlink current timestreams
            m_dynamoCluster.networker->link_timestreams(nullptr);
        }

        // EXTRACTION COMPLETE!

        {
            const std::lock_guard<std::mutex> rLk(m_runtimeMux);
            
            m_currentTimeslice = NULL_TIMESLICEID;

            // if this is timeslice 0 AND m_isRecycleNeeded is true,
            //   then send new init request to past-most timeslice
            if (dstCosmos->get_host_id() == 0U && m_isRecycleNeeded)
            {
                PLEEPLOG_DEBUG("Finished extraction; starting recycle");
                EventMessage initMessage(events::parallel::INIT);
                events::parallel::INIT_params initInfo{ static_cast<uint16_t>(m_pastmostTimeslice) };
                initMessage << initInfo;
                m_eventBroker->send_event(initMessage);

                m_isRecycleNeeded = false;
                m_currentState = State::initializing;
            }
            // otherwise send init request to same timesliceId until we reach timeslice 0
            else if (dstCosmos->get_host_id() > 0U)
            {
                PLEEPLOG_DEBUG("Finished extraction; moving to next timeslice");
                EventMessage initMessage(events::parallel::INIT);
                events::parallel::INIT_params initInfo{ dstCosmos->get_host_id() };
                initMessage << initInfo;
                m_eventBroker->send_event(initMessage);

                m_currentState = State::initializing;
            }
            // otherwise we're done until later requests
            else
            {
                PLEEPLOG_DEBUG("Finished extraction; going idle");
                // any cleanup needed at end of parallel cyclings:

                // just incase there are leftovers somehow
                m_readingSteinerEntities.clear();
                m_interceptionHistory.clear();

                m_currentState = State::idle;
            }
        }

        return true;
    }

    void ParallelCosmosContext::_divergence_handler(EventMessage divEvent)
    {
        events::parallel::DIVERGENCE_params divInfo;
        divEvent >> divInfo;

        // need lock because this handler can also be used by the api
        const std::lock_guard<std::mutex> rLk(m_runtimeMux);

        PLEEPLOG_DEBUG("Timeslice " + std::to_string(divInfo.sourceTimeslice) + " has a divergence, we are " + std::to_string(m_currentState) + " on parallel timeslice " + std::to_string(m_currentTimeslice));

        if (m_currentState == State::idle)
        {
            // send init request immediately via event to network dynamo
            EventMessage initMessage(events::parallel::INIT);
            // always start from beginning?
            events::parallel::INIT_params initInfo{ static_cast<uint16_t>(m_pastmostTimeslice) }; 
            //events::parallel::INIT_params initInfo{ divInfo.sourceTimeslice };
            initMessage << initInfo;
            m_eventBroker->send_event(initMessage);

            m_currentState = State::initializing;
        }
        /// TODO: this will trigger if it is caught between extract and init while timeslice is null (null == 14 >= sourceTimeslice)
        else if (m_currentTimeslice >= divInfo.sourceTimeslice || divInfo.sourceTimeslice == NULL_TIMESLICEID)
        {
            m_isRecycleNeeded = true;
        }
        // if we are simulating the past of the divergent timeslice then we will get there in the current cycle
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
    
    void ParallelCosmosContext::_entity_created_handler(EventMessage creationEvent)
    {
        // removed from running condemned list if it has been set

        events::cosmos::ENTITY_CREATED_params creationInfo;
        creationEvent >> creationInfo;

        m_condemnedEntities.erase(creationInfo.entity);
    }

    void ParallelCosmosContext::_worldline_shift_handler(EventMessage shiftEvent)
    {
        events::parallel::WORLDLINE_SHIFT_params shiftInfo;
        shiftEvent >> shiftInfo;

        m_readingSteinerEntities.insert(shiftInfo.entity);
    }
    
    void ParallelCosmosContext::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;

        // cache interaction from agent -> recipient
        PLEEPLOG_DEBUG("RECORDING interception from " + std::to_string(interceptionInfo.agent) + " to " + std::to_string(interceptionInfo.recipient));
        m_interceptionHistory[interceptionInfo.agent].push(interceptionInfo.recipient);
        m_interceptionHistory[interceptionInfo.recipient].push(interceptionInfo.agent);
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

            m_currentState = State::ready;

            return;
        }
        
        // ensure state is busy while running
        m_currentState = State::busy;

        // artifically give ample time to reach target without waiting
        // pretend we are always behind in simulation time, and need to catch up
        m_fixedTimeRemaining = m_fixedTimeStep * 2.0;

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