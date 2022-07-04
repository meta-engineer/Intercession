#include "server_cosmos_context.h"

namespace pleep
{
    ServerCosmosContext::ServerCosmosContext() 
        : CosmosContext()
        //, m_controlDynamo(nullptr)
        , m_physicsDynamo(nullptr)
        , m_networkDynamo(nullptr)
    {
        // CosmosContext() has setup broker
        
        // construct dynamos
        //m_controlDynamo = new ControlDynamo(m_eventBroker, windowApi);
        m_physicsDynamo = new PhysicsDynamo(m_eventBroker);
        // TODO: server specific network dynamo
        m_networkDynamo = new NetworkDynamo(m_eventBroker);
        
        // build empty starting cosmos
        m_currentCosmos = new Cosmos();
        
        // populate starting cosmos (without rendering?)
        // eventually we'll pass some config param here
        //_build_cosmos();
    }
    
    ServerCosmosContext::~ServerCosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;
        
        delete m_networkDynamo;
        delete m_physicsDynamo;
        //delete m_controlDynamo;
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
}