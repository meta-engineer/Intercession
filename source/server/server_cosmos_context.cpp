#include "server_cosmos_context.h"

#include "staging/test_cosmos.h"
#include "staging/iceberg_cosmos.h"

namespace pleep
{
    ServerCosmosContext::ServerCosmosContext(TimelineApi localTimelineApi) 
        : I_CosmosContext()
    {
        // I_CosmosContext() has setup broker (not shared between contexts)
        
        // construct dynamos
        m_dynamoCluster.networker = std::make_shared<ServerNetworkDynamo>(m_eventBroker, localTimelineApi);
        m_dynamoCluster.behaver  = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);
        
        // build and populate starting cosmos
        // eventually we'll pass some cosmos config param here
        if (m_dynamoCluster.networker->get_timeslice_id() == 0)
        {
            _build_cosmos();
        }
        else
        {
            // TODO: past servers need to receive config from their parent, like clients
            m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);

            // set coherency very far behind 
            // (65535 - 40000)/90hz = 283 seconds
            // and wait for coherency sync
            //m_currentCosmos->set_coherency(40000);
            
            // TEMP: delay Cosmos' coherency into the past according to our timeslice id
            // assume all servers will start at once
            m_currentCosmos->set_coherency(0 - 
                static_cast<uint16_t>(localTimelineApi.get_timeslice_delay() *
                                      localTimelineApi.get_timeslice_id())
            );
        }
    }
    
    ServerCosmosContext::~ServerCosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences?
        m_currentCosmos = nullptr;
    }

    void ServerCosmosContext::_prime_frame() 
    {
        // ***** Cosmos Update *****
        // invokes all registered synchros to process their entities
        // this fills the dynamo relays with packets which should be cleared in _clean_frame()
        if (m_currentCosmos) m_currentCosmos->update();
    }
    
    void ServerCosmosContext::_on_fixed(double fixedTime) 
    {
        // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
        //   know which dynamos to call fixed and which to call on frametime
        m_dynamoCluster.networker->run_relays(fixedTime);
        m_dynamoCluster.behaver->run_relays(fixedTime);
        m_dynamoCluster.physicser->run_relays(fixedTime);
    }
    
    void ServerCosmosContext::_on_frame(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void ServerCosmosContext::_clean_frame() 
    {
        // flush dynamos of all synchro submissions
        m_dynamoCluster.networker->reset_relays();
        m_dynamoCluster.behaver->reset_relays();
        m_dynamoCluster.physicser->reset_relays();
    }

    void ServerCosmosContext::_build_cosmos()
    {
        PLEEPLOG_TRACE("Start cosmos construction");

        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        m_currentCosmos = build_iceberg_cosmos(m_eventBroker, m_dynamoCluster);
        //m_currentCosmos = build_test_temporal_cosmos(m_eventBroker, m_dynamoCluster);

        PLEEPLOG_TRACE("Done cosmos construction");
    }
}