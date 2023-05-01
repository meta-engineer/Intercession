#include "server_cosmos_context.h"

#include "staging/test_cosmos.h"
#include "staging/test_temporal_cosmos.h"

namespace pleep
{
    ServerCosmosContext::ServerCosmosContext(TimelineApi localTimelineApi) 
        : I_CosmosContext()
    {
        // I_CosmosContext() has setup broker (not shared between contexts)
        
        // construct dynamos
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);
        m_dynamoCluster.networker = std::make_shared<ServerNetworkDynamo>(m_eventBroker, localTimelineApi);
        m_dynamoCluster.scripter  = std::make_shared<ScriptDynamo>(m_eventBroker);
        
        // build and populate starting cosmos
        // eventually we'll pass some cosmos config param here
        _build_cosmos();

        // Send test entity to clients
        // flag entity as updated once here:
        //m_eventBroker->send_event(...);
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
        m_currentCosmos->update();
    }
    
    void ServerCosmosContext::_on_fixed(double fixedTime) 
    {
        // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
        //   know which dynamos to call fixed and which to call on frametime
        m_dynamoCluster.networker->run_relays(fixedTime);
        m_dynamoCluster.physicser->run_relays(fixedTime);
        m_dynamoCluster.scripter->run_relays(fixedTime);
    }
    
    void ServerCosmosContext::_on_frame(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void ServerCosmosContext::_clean_frame() 
    {
        // flush dynamos of all synchro submissions
        m_dynamoCluster.scripter->reset_relays();
        m_dynamoCluster.networker->reset_relays();
        m_dynamoCluster.physicser->reset_relays();
    }

    void ServerCosmosContext::_build_cosmos()
    {
        PLEEPLOG_TRACE("Start cosmos construction");

        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        m_currentCosmos = build_test_cosmos(m_eventBroker, m_dynamoCluster);
        //m_currentCosmos = build_temporal_cosmos(m_eventBroker, m_dynamoCluster);

        PLEEPLOG_TRACE("Done cosmos construction");
    }
}