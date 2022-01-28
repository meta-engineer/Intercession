#include "cosmos.h"

#include "logging/pleep_log.h"

namespace pleep
{
    // ECS methods are provided inline in cosmos.h

    Cosmos::Cosmos() 
    {
        m_entityRegistry    = std::make_unique<EntityRegistry>();
        m_componentRegistry = std::make_unique<ComponentRegistry>();
        m_synchroRegistry   = std::make_unique<SynchroRegistry>();
    }

    Cosmos::~Cosmos() 
    {
        // registry smart pointers cleared 
    }
    
    void Cosmos::update(double deltaTime) 
    {
        // get synchro map from registry
        auto synchroIter = m_synchroRegistry->get_synchros_ref().begin();
        auto synchroEnd  = m_synchroRegistry->get_synchros_ref().end();

        // update all registered synchros
        for (synchroIter; synchroIter != synchroEnd; synchroIter++)
        {
            // we can only call ISynchro methods
            // otherwise Context will have to keep and call each specialized synchro
            // context should only need to manage its dynamos
            synchroIter->second->update(deltaTime);
        }
    }    

}