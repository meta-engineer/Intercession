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

        // set any specific configs for headless simulation
        // we want to simulate at same timestep as caller, but not in realtime!
        // pretend we are always behind in simulation time, and ned to catch up
        m_timeRemaining = std::chrono::duration<double>(2147483647);

        // event handlers

    }
    
    void ParallelCosmosContext::injectMessage(EventMessage msg) 
    {
        if (m_isRunning)
        {
            PLEEPLOG_WARN("Parallel Context cannot accept injected events while running, received: " + std::to_string(msg.header.id));
            return;
        }

        // mimic event broker handlers
        switch(msg.header.id)
        {
        case events::cosmos::ENTITY_CREATED:
        {
            events::cosmos::ENTITY_CREATED_params createInfo;
            msg >> createInfo;

            // parent context injected a new entity, lets create it
            PLEEPLOG_DEBUG("Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

            if (m_currentCosmos->register_entity(createInfo.entity))
            {
                for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                {
                    if (createInfo.sign.test(c)) m_currentCosmos->add_component(createInfo.entity, c);
                }
            }
        }
        break;
        case events::cosmos::ENTITY_UPDATE:
        { 
            events::cosmos::ENTITY_UPDATE_params updateInfo;
            msg >> updateInfo;

            // parent context updated an entity, lets apply it
            
            // ensure entity exists
            if (m_currentCosmos->entity_exists(updateInfo.entity))
            {
                // read update into Cosmos
                m_currentCosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, updateInfo.subset, msg);
            }
            else
            {
                PLEEPLOG_ERROR("Received ENTITY_UPDATE for entity " + std::to_string(updateInfo.entity) + " which does not exist, skipping...");
            }
        }
        break;
        default:
        {
            PLEEPLOG_WARN("Parallel context has no handler for injected message of type: " + std::to_string(msg.header.id));
        }
        }
    }
    
    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        // needs a lock?
        m_coherencyTarget = coherency;
    }
    
    void ParallelCosmosContext::_prime_frame() 
    {
        if (m_currentCosmos) m_currentCosmos->update();
    }
    
    void ParallelCosmosContext::_on_fixed(double fixedTime) 
    {
        // check if we reached our target before running next step
        if (m_currentCosmos && m_currentCosmos->get_coherency() >= m_coherencyTarget)
        {
            // fake finishing this frame
            m_timeRemaining = std::chrono::duration<double>(0);
            // don't run next frame
            this->stop();
        }

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
    
    void ParallelCosmosContext::_entity_created_handler(EventMessage entityEvent) 
    {
    }
}