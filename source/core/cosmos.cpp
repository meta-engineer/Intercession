#include "cosmos.h"

#include "logging/pleep_log.h"

namespace pleep
{
    // ECS methods are provided inline in cosmos.h

    Cosmos::Cosmos(EventBroker* sharedBroker, const TimesliceId localTimesliceIndex)
    {
        m_entityRegistry    = std::make_unique<EntityRegistry>(localTimesliceIndex);
        m_componentRegistry = std::make_unique<ComponentRegistry>();
        m_synchroRegistry   = std::make_unique<SynchroRegistry>();

        m_sharedBroker = sharedBroker;
        // setup handlers?
    }

    Cosmos::~Cosmos() 
    {
        // registry smart pointers cleared 
    }
    
    void Cosmos::update() 
    {
        // get synchro map from registry
        auto synchroIter = m_synchroRegistry->get_synchros_ref().begin();
        auto synchroEnd  = m_synchroRegistry->get_synchros_ref().end();

        // update all registered synchros
        for (synchroIter; synchroIter != synchroEnd; synchroIter++)
        {
            // we can only call I_Synchro methods
            // otherwise Context will have to keep and call each specialized synchro
            // context should only need to manage its dynamos
            synchroIter->second->update();
        }
    }

    void Cosmos::serialize_entity_components(Entity entity, EventMessage msg)
    {
        Signature entitySign = this->get_entity_signature(entity);
        m_componentRegistry->serialize_entity_components(entity, entitySign, msg);
    }

    void Cosmos::deserialize_and_write_component(Entity entity, std::string componentName, EventMessage msg)
    {
        m_componentRegistry->deserialize_and_write_component(entity, componentName, msg);
    }
}