#ifndef COMPONENT_REGISTRY_H
#define COMPONENT_REGISTRY_H

//#include "intercession_pch.h"
#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <exception>

#include "ecs_types.h"
#include "component_array.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class ComponentRegistry
    {
    public:
        // add templated type to known component types in the registry
        // THROWS runtime_error if component is already registered
        template<typename T>
        void register_component_type();

        // get registered component index id for templated type
        // THROWS runtime_error if component type is not yet registered
        template<typename T>
        ComponentType get_component_type();

        // add component to registry array for given type and entity
        // THROWS runtime_error if templated type has not yet been registered
        template<typename T>
        void add_component(Entity entity, T component);

        // remove component from registry array for given type and entity
        // THROWS runtime_error if templated type has not yet been registered
        template<typename T>
        void remove_component(Entity entity);
        
        // get reference to specific component of given type for entity
        // THROWS if component does not exist or component type has not yet been registered
        template<typename T>
        T& get_component(Entity entity);
        
        // safely clear all registered components for given entity
        void clear_entity(Entity entity);

        // return entity with component "equal" to argument
        // operator == must be defined for T
        template<typename T>
        Entity find_entity(T component);

    private:
        // cast ComponentArray into mapped type
        template<typename T>
        std::shared_ptr<ComponentArray<T>> _get_component_array();

        // "type string" pointer to a component type's id
        std::unordered_map<const char*, ComponentType> m_componentTypes{};

        // "type string" pointer to a component Array objeect pointer
        std::unordered_map<const char*, std::shared_ptr<I_ComponentArray>> m_componentArrays{};

        // track total components registered
        ComponentType m_componentTypeCount;
    };

    
    template<typename T>
    void ComponentRegistry::register_component_type()
    {
        const char* typeName = typeid(T).name();

        if (m_componentTypes.find(typeName) != m_componentTypes.end())
        {
            PLEEPLOG_ERROR("Cannot register component type " + std::string(typeName) + " which already exists");
            throw std::runtime_error("ComponentRegistry cannot register component type " + std::string(typeName) + " which already exists");
        }

        // add type id/name to next register index
        m_componentTypes.insert({typeName, m_componentTypeCount});

        // create an array object for this type id/name
        m_componentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});

        // Increment count to next available index
        m_componentTypeCount++;
    }
    
    template<typename T>
    ComponentType ComponentRegistry::get_component_type()
    {
        const char* typeName = typeid(T).name();

        if (m_componentTypes.find(typeName) == m_componentTypes.end())
        {
            PLEEPLOG_ERROR("Cannot retrieve component type " + std::string(typeName) + " which has not been registered");
            throw std::range_error("ComponentRegistry cannot retrieve component type " + std::string(typeName) + " which has not been registered");
        }

        // this type is used for creating signatures
        return m_componentTypes[typeName];
    }

    template<typename T>
    void ComponentRegistry::add_component(Entity entity, T component)
    {
        this->_get_component_array<T>()->insert_data_for(entity, component);
    }

    template<typename T>
    void ComponentRegistry::remove_component(Entity entity)
    {
        this->_get_component_array<T>()->remove_data_for(entity);
    }
    
    template<typename T>
    T& ComponentRegistry::get_component(Entity entity)
    {
        return this->_get_component_array<T>()->get_data_for(entity);
    }
    
    inline void ComponentRegistry::clear_entity(Entity entity)
    {
        // typeid & ComponentArray pointer
        for (auto const& pair : m_componentArrays)
        {
            //std::shared_ptr<I_ComponentArray> const& components = pair.second;
            pair.second->clear_data_for(entity);
        }
    }
    
    template<typename T>
    Entity ComponentRegistry::find_entity(T component)
    {
        return this->_get_component_array<T>()->find_entity_for(component);
    }
    
    template<typename T>
    std::shared_ptr<ComponentArray<T>> ComponentRegistry::_get_component_array()
    {
        // c++ magic?
        const char* typeName = typeid(T).name();

        // component type may not have been registered
        if (m_componentTypes.find(typeName) == m_componentTypes.end())
        {
            PLEEPLOG_ERROR("Cannot get array for component " + std::string(typeName) + " which has not been registered");
            throw std::runtime_error("ComponentArray cannot get array for component " + std::string(typeName) + " which has not been registered");
        }

        return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]);
    }
}

#endif // COMPONENT_REGISTRY_H