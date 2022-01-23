#ifndef STATOR_H
#define STATOR_H

//#include "intercession_pch.h"
#include <memory>

#include "ecs_types.h"
#include "entity_registry.h"
#include "component_registry.h"
#include "synchro_registry.h"

namespace pleep
{
    class Stator
    {
    public:
        // build owned ECS modules
        void init();

        // call through methods to ECS modules

        Entity create_entity();
        void destroy_entity(Entity entity);

        template<typename T>
        void register_component();
        
        template<typename T>
        void add_component(Entity entity, T component);

        template<typename T>
        void remove_component(Entity entity);
        
        template<typename T>
        T& get_component(Entity entity);

        template<typename T>
        ComponentType get_component_type();
        
        template<typename T>
        std::shared_ptr<T> register_synchro();
        
        template<typename T>
        void set_synchro_signature(Signature sign);

    private:
        std::unique_ptr<ComponentRegistry> m_componentRegistry;
        std::unique_ptr<EntityRegistry>    m_entityRegistry;
        std::unique_ptr<SynchroRegistry>   m_synchroRegistry;       
    };


    void Stator::init()
    {
        m_entityRegistry    = std::make_unique<EntityRegistry>();
        m_componentRegistry = std::make_unique<ComponentRegistry>();
        m_synchroRegistry   = std::make_unique<SynchroRegistry>();
    }

    Entity Stator::create_entity()
    {
        return m_entityRegistry->create_entity();
    }

    void Stator::destroy_entity(Entity entity)
    {
        m_entityRegistry->destroy_entity(entity);

        m_componentRegistry->clear_entity(entity);
        m_synchroRegistry->clear_entity(entity);
    }

    template<typename T>
    void Stator::register_component()
    {
        m_componentRegistry->register_component_type<T>();
    }
    
    template<typename T>
    void Stator::add_component(Entity entity, T component)
    {
        m_componentRegistry->add_component<T>(entity, component);

        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), true);
        m_entityRegistry->set_signature(entity, sign);

        m_synchroRegistry->change_entity_signature(entity, sign);

    }

    template<typename T>
    void Stator::remove_component(Entity entity)
    {
        m_componentRegistry->remove_component<T>(entity);
        
        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), false);
        m_entityRegistry->set_signature(entity, sign);

        m_synchroRegistry->change_entity_signature(entity, sign);
    }
    
    template<typename T>
    T& Stator::get_component(Entity entity)
    {
        return m_componentRegistry->get_component<T>(entity);
    }

    template<typename T>
    ComponentType Stator::get_component_type()
    {
        return m_componentRegistry->get_component_type<T>();
    }
    
    template<typename T>
    std::shared_ptr<T> Stator::register_synchro()
    {
        return m_synchroRegistry->register_synchro<T>();
    }
    
    template<typename T>
    void Stator::set_synchro_signature(Signature sign)
    {
        m_synchroRegistry->set_signature<T>(sign);
    }
}

#endif // STATOR_H