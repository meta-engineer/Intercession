#include "server_cosmos_context.h"

#include "staging/test_temporal_cosmos.h"

namespace pleep
{
    ServerCosmosContext::ServerCosmosContext(TimelineApi localTimelineApi) 
        : I_CosmosContext()
        //, m_scriptDynamo(nullptr)
        , m_physicsDynamo(nullptr)
        , m_networkDynamo(nullptr)
    {
        // I_CosmosContext() has setup broker (not shared between contexts)
        
        // construct dynamos
        m_physicsDynamo = new PhysicsDynamo(m_eventBroker);
        m_networkDynamo = new ServerNetworkDynamo(m_eventBroker, localTimelineApi);
        
        // build empty starting cosmos
        // if cosmos is rebuilt, it will need timeslice id again
        // how can a context be called to delete and remake a context? or would the whole context be rebuilt?
        m_currentCosmos = new Cosmos(m_eventBroker, m_networkDynamo->get_timeslice_id());
        
        // populate starting cosmos
        // eventually we'll pass some cosmos config param here
        _build_cosmos();
    }
    
    ServerCosmosContext::~ServerCosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;
        
        delete m_networkDynamo;
        delete m_physicsDynamo;
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
        m_networkDynamo->run_relays(fixedTime);
        m_physicsDynamo->run_relays(fixedTime);
    }
    
    void ServerCosmosContext::_on_frame(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void ServerCosmosContext::_clean_frame() 
    {
        // flush dynamos of all synchro submissions
        m_networkDynamo->reset_relays();
        m_physicsDynamo->reset_relays();        
    }

    void ServerCosmosContext::_build_cosmos()
    {
        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        build_temporal_cosmos(m_currentCosmos, m_eventBroker, m_physicsDynamo, m_networkDynamo);
    }
}