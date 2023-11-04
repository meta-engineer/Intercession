#include "parallel_cosmos_context.h"

#include "staging/hard_config_cosmos.h"

namespace pleep
{
    ParallelCosmosContext::ParallelCosmosContext()
        : I_CosmosContext()
    {
        // dynamos should only be for headless simulation
        m_dynamoCluster.behaver  = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);

        // TEMP: use hard-coded cosmos config
        m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);

        // event handlers

        // Listen for interception events, and store forked entities for retreival
        m_eventBroker->add_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));
    }

    ParallelCosmosContext::~ParallelCosmosContext()
    {
        m_eventBroker->remove_listener(METHOD_LISTENER(events::cosmos::TIMESTREAM_INTERCEPTION, ParallelCosmosContext::_timestream_interception_handler));
    }
    
    void ParallelCosmosContext::copy_cosmos(const std::shared_ptr<Cosmos> cosmos)
    {
        // any previous forked entities are invalid
        m_forkedEntities.clear();

        m_currentCosmos->set_coherency(cosmos->get_coherency());
        /////////////////////////////////////////////////////////////// TODO

        // set initial forked entities into m_forkedEntities

        return;
    }
    
    void ParallelCosmosContext::copy_timestreams(const std::shared_ptr<EntityTimestreamMap> timestreams)
    {
        /////////////////////////////////////////////////////////////// TODO

        return;
    }

    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        // needs a lock?
        m_coherencyTarget = coherency;
        // give ample time to reach target without waiting
        // pretend we are always behind in simulation time, and need to catch up
        m_timeRemaining = std::chrono::duration<double>(9999.9);
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
        if (!m_currentCosmos || (m_currentCosmos && !m_currentCosmos->entity_exists(e))) return false;

        m_currentCosmos->serialize_entity_components(e, m_currentCosmos->get_entity_signature(e), dst);
        return true;
    }

    void ParallelCosmosContext::_timestream_interception_handler(EventMessage interceptionEvent)
    {
        events::cosmos::TIMESTREAM_INTERCEPTION_params interceptionInfo;
        interceptionEvent >> interceptionInfo;
        
        m_forkedEntities.push_back(interceptionInfo.recipient);
    }


    void ParallelCosmosContext::_prime_frame() 
    {
        if (!m_currentCosmos) return;

        // check if we reached our target before running next step
        if (m_currentCosmos->get_coherency() >= m_coherencyTarget)
        {
            PLEEPLOG_DEBUG("Parallel reached coherency target of " + std::to_string(m_coherencyTarget));
            // don't run this frame
            m_timeRemaining = std::chrono::duration<double>(-2147483647);
            // don't run next frame
            this->stop();
        }

        ///////////////////// TODO Deserialize upstream components from timestream copy

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