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
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::CONDEMN_ENTITY, Cosmos::_condemn_entity_handler));
    }

    Cosmos::~Cosmos() 
    {
        // registry smart pointers cleared

        // clear handlers
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::cosmos::CONDEMN_ENTITY, Cosmos::_condemn_entity_handler));
    }
    
    void Cosmos::update() 
    {
        // delete all condemned entities
        for (Entity e : m_condemned)
        {
            this->destroy_entity(e);
        }
        m_condemned.clear();

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

    void Cosmos::serialize_entity_components(Entity entity, Signature sign, EventMessage& msg)
    {
        Signature entitySign = this->get_entity_signature(entity);
        // get components ONLY in sign
        if (((entitySign ^ sign) & sign).any())
        {
            PLEEPLOG_ERROR("Requested serialized signature (" + sign.to_string() + ") which is a superset of entity (" + std::to_string(entity) + ") signature (" + entitySign.to_string() + ")");
            throw std::range_error("Signature mismatch for requested serialization");
        }
        m_componentRegistry->serialize_entity_components(entity, sign, msg);
    }

    void Cosmos::deserialize_entity_components(Entity entity, Signature sign, EventMessage& msg)
    {
        Signature entitySign = this->get_entity_signature(entity);
        // get components ONLY in sign
        if (((entitySign ^ sign) & sign).any())
        {
            PLEEPLOG_ERROR("Requested deserialized signature (" + sign.to_string() + ") which is a superset of entity (" + std::to_string(entity) + ") signature (" + entitySign.to_string() + ")");
            throw std::range_error("Signature mismatch for requested deserialization");
        }
        m_componentRegistry->deserialize_entity_components(entity, sign, msg);
    }

    void Cosmos::deserialize_single_component(Entity entity, ComponentType type, EventMessage& msg)
    {
        m_componentRegistry->deserialize_single_component(entity, type, msg);
    }

    bool Cosmos::set_focal_entity(Entity entity)
    {
        if (!this->entity_exists(entity))
        {
            return false;
        }

        m_focalEntity = entity;
        return true;
    }

    Entity Cosmos::get_focal_entity()
    {
        if (!this->entity_exists(m_focalEntity))
        {
            m_focalEntity = NULL_ENTITY;
        }

        return m_focalEntity;
    }
    
    void Cosmos::increment_coherency()
    {
        m_stateCoherency++;
    }
    uint16_t Cosmos::get_coherency()
    {
        return m_stateCoherency;
    }
    void Cosmos::set_coherency(uint16_t gotoCoherency)
    {
        m_stateCoherency = gotoCoherency;
    }

    
    std::vector<std::string> Cosmos::stringify_synchro_registry() 
    {
        return m_synchroRegistry->stringify();
    }
    
    std::vector<std::string> Cosmos::stringify_component_registry() 
    {
        return m_componentRegistry->stringify();
    }
    
    void Cosmos::_condemn_entity_handler(EventMessage condemnEvent) 
    {
        events::cosmos::CONDEMN_ENTITY_params condemnInfo;
        condemnEvent >> condemnInfo;
        
        PLEEPLOG_TRACE("Entity " + std::to_string(condemnInfo.entity) + " was condemned to deletion.");
        m_condemned.insert(condemnInfo.entity);
    }
}