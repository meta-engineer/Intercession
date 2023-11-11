#include "parallel_cosmos_context.h"

#include "staging/hard_config_cosmos.h"
#include "server/server_network_dynamo.h"

namespace pleep
{
    ParallelCosmosContext::ParallelCosmosContext()
        : I_CosmosContext()
    {
        // dynamos should only be for headless simulation
        m_dynamoCluster.behaver  = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);
 
        // event handlers

        // Listen for interception events, and store forked entities for retreival
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));

        /// TODO: also handle entity creation/deletion events so that they are extractable into the local cosmos
    }

    ParallelCosmosContext::~ParallelCosmosContext()
    {
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));
    }
    
    void ParallelCosmosContext::init_cosmos(const std::shared_ptr<Cosmos> sourceCosmos, const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams, const std::unordered_set<Entity>& resolutionCandidates, const std::unordered_set<Entity>& nonCandidates)
    {
        ////////////////////////////// TODO: setup cosmos with hostTimesliceID without needing a ServerNetworkDynamo? or using a specialized ParallelNetworkDynamo?
        // TEMP: use hard-coded cosmos config
        m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);

        m_currentCosmos->set_coherency(sourceCosmos->get_coherency());


        // I can't just copy cosmos as it is because any time travellers aren't in the future...
        // I need to copy the cosmos as it is in the timestream, 
        // EXCEPT the resolution candidates need to be how they are in the cosmos directly...
        // so that excludes any forked entities that AREN'T candidates, and and time-travellers

        // alternatively I can copy the cosmos as it is now, BUT:
        // - delete/omit any entities which do not have a future timestream (time-travellers)
        // - remove Spacetimecomponent of all entities which weren't marked as candidates


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

            // if i'm not a candidate, BUT I have a spacetime component (and are forked) 
            //      then I want to use my timestream version instead?
            // (TODO: can I track this entity set in Superposition relay aswell?)
            // I need to parse through the timestream (which has been ignored since this entity got forked)
            // and find the message with the same coherency as current 
            //      (assuming timstream range is > FORKED_THREASHOLD_MIN)
            if (nonCandidates.count(signMapIt.first))
            {
                EventMessage trueUpdate(events::cosmos::ENTITY_UPDATE);
                while (sourceFutureTimestreams->pop_from_timestream(signMapIt.first, sourceCosmos->get_coherency(), trueUpdate))
                {
                    // keep popping until we reach the current coherency
                    if (trueUpdate.header.coherency == m_currentCosmos->get_coherency())
                    {
                        // use this update for this entity instead
                        // this should remove its forked state
                        m_currentCosmos->deserialize_entity_components(
                            signMapIt.first,
                            signMapIt.second,
                            false,
                            trueUpdate
                        );
                        break;
                    }
                    // we should always start behidn the current coherency
                    assert(trueUpdate.header.coherency < sourceCosmos->get_coherency());
                }
            }
        }

        // Build m_forkedEntities from initial forked entities
        // so that they can be extracted at end of simulation
        // fastest would be copy passed m_resolutionCandidates from superposition relay?
        // any previous forked entities are invalid
        m_forkedEntities.clear();
        for (Entity e : resolutionCandidates) {
            PLEEPLOG_DEBUG("Noting inital forked entity " + std::to_string(e));
            m_forkedEntities.push_back(e);
        }

        
        /// TODO: also need to setup timestream to feed upstream components into local cosmos

        // usually ServerNetworkDynamo run_relays does this
        // (as it is monolithic right now, not actually using relays)
        // so ideally we are able to reuse that by creating a benign network dynamo
        // which may require actually seperating the relays in ServerNetworkDynamo


    }

    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        // needs a lock?
        m_coherencyTarget = coherency;
    }

    uint16_t ParallelCosmosContext::get_current_coherency()
    {
        return (m_currentCosmos && !this->is_running()) ? m_currentCosmos->get_coherency() : 0;
    }

    const std::vector<Entity> ParallelCosmosContext::get_forked_entities()
    {
        return m_forkedEntities;
    }

    bool ParallelCosmosContext::extract_entity(Entity e, EventMessage& dst)
    {
        PLEEPLOG_DEBUG("Try to extract: " + std::to_string(e));
        if (!(m_currentCosmos && m_currentCosmos->entity_exists(e))) return false;

        events::cosmos::ENTITY_UPDATE_params updateInfo{
            e,
            m_currentCosmos->get_entity_signature(e),
            false
        };

        // expects dst to be empty
        m_currentCosmos->serialize_entity_components(updateInfo.entity, updateInfo.sign, dst);
        dst << updateInfo;
        dst.header.id = events::cosmos::ENTITY_UPDATE;
        return true;
    }

    void ParallelCosmosContext::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;

        // set forked state for recipient
        if (!m_currentCosmos->has_component<SpacetimeComponent>(interceptionInfo.recipient))
        {
            SpacetimeComponent newSpacetime;
            m_currentCosmos->add_component<SpacetimeComponent>(interceptionInfo.recipient, newSpacetime);
        }
        
        SpacetimeComponent& oldSpacetime = m_currentCosmos->get_component<SpacetimeComponent>(interceptionInfo.recipient);
        oldSpacetime.timestreamState = TimestreamState::forked;
        oldSpacetime.timestreamStateCoherency = m_currentCosmos->get_coherency();
        
        // mark as forked for extraction at end of simulation
        m_forkedEntities.push_back(interceptionInfo.recipient);
    }


    void ParallelCosmosContext::_prime_frame() 
    {
        if (!m_currentCosmos) return;

        // check if we reached our target before running next step
        if (m_currentCosmos->get_coherency() >= m_coherencyTarget)
        {
            PLEEPLOG_DEBUG("Parallel reached coherency target of " + std::to_string(m_currentCosmos->get_coherency()));
            // coherency is checked by local context BEFORE physics updates that frame
            // so we want to exist BEFORE our physics updates in _on_fixed
            // don't run this frame
            m_timeRemaining = std::chrono::duration<double>(-2147483647);
            // don't run next frame
            this->stop();
            return;
        }

        // artifically give ample time to reach target without waiting
        // pretend we are always behind in simulation time, and need to catch up
        m_timeRemaining = m_fixedTimeStep;

        ///////////////////////////////////////////////////////////////// TODO
        // Deserialize upstream components from timestream copy

        m_currentCosmos->update();
    }
    
    void ParallelCosmosContext::_on_fixed(double fixedTime) 
    {
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
        m_dynamoCluster.behaver->reset_relays();
        m_dynamoCluster.physicser->reset_relays();
    }
}