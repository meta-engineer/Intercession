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

        // TODO: Listen for interception events, and store forked entities for retreival
    }
    
    void ParallelCosmosContext::copy_cosmos(const std::shared_ptr<Cosmos> cosmos)
    {
        // TODO
        return;
    }
    
    void ParallelCosmosContext::copy_timestreams(const std::shared_ptr<EntityTimestreamMap> timestreams)
    {
        // TODO
        return;
    }

    void ParallelCosmosContext::close()
    {
        // ensure thread has also joined?
        // TODO
        return;
    }

    bool ParallelCosmosContext::cosmos_exists()
    {
        // TODO
        // move this into I_CosmosContext???
        return false;
    }

    void ParallelCosmosContext::start()
    {
        // TODO
        // move this into I_CosmosContext???
        return;
    }

    void ParallelCosmosContext::join()
    {
        stop();
        // TODO
        return;
    }

    void ParallelCosmosContext::set_coherency_target(uint16_t coherency) 
    {
        // needs a lock?
        m_coherencyTarget = coherency;
    }

    uint16_t ParallelCosmosContext::get_current_coherency()
    {
        // be sure to expect a return of 0 in leiu of checking is_running()
        return (m_currentCosmos && !this->is_running()) ? m_currentCosmos->get_coherency() : 0;
    }

    const std::vector<Entity> ParallelCosmosContext::get_forked_entities()
    {
        // TODO
        return std::vector<Entity>{};
    }

    EventMessage ParallelCosmosContext::extract_entity(Entity e)
    {
        // TODO
        return EventMessage{};
    }



    void ParallelCosmosContext::_prime_frame() 
    {
        // check if we reached our target before running next step
        if (m_currentCosmos && m_currentCosmos->get_coherency() >= m_coherencyTarget)
        {
            // fake finishing this frame
            m_timeRemaining = std::chrono::duration<double>(-2147483647);
            // don't run next frame
            this->stop();
        }

        if (m_currentCosmos) m_currentCosmos->update();
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