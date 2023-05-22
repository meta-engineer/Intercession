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
#include "events/event_types.h"

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

        // get registered component name from its index id
        // THROWS runtime_error if component type is not yet registered
        const char* get_component_name(ComponentType componentId);

        // add component to registry array for given type and entity
        // THROWS runtime_error if templated type has not yet been registered
        template<typename T>
        void add_component(Entity entity, T component);
        
        // add default constructed component of given ID
        // THROWS runtime_error if componentId does not exist
        void add_component(Entity entity, ComponentType componentId);

        // remove component from registry array for given type and entity
        // THROWS runtime_error if templated type has not yet been registered
        template<typename T>
        void remove_component(Entity entity);

        // remove component of given ID
        // THROWS runtime_error if componentId does not exist
        void remove_component(Entity entity, ComponentType componentId);
        
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

        
        ///// Helper methods for sending entity information in Messages (for events or network) /////

        // loop through each ComponentType in entitySign, map to name, index that component array,
        // get the component data for entity, then push it into message
        void serialize_entity_components(Entity entity, Signature entitySign, EventMessage& msg);

        // loop through each ComponentType in entitySign, map to name, index that component array,
        // call deserialize_and_write_component with that component
        void deserialize_entity_components(Entity entity, Signature entitySign, EventMessage& msg);

        // index component array using name,
        // get that array to unpack next data in msg, and copy it into entity's data
        // *** componentName MUST BE THE pointer from typeid!!! use get_component_name()!!!
        void deserialize_and_write_component(Entity entity, const char* componentTypename, EventMessage& msg);

        // Return list of component typeid names
        // MUST be ordered by ComponentType
        std::vector<std::string> stringify();

    private:
        // cast ComponentArray into mapped type
        template<typename T>
        std::shared_ptr<ComponentArray<T>> _get_component_array();

        // "type string" pointer to a component type's id
        std::unordered_map<const char*, ComponentType> m_componentTypes{};
        // reverse map
        std::unordered_map<ComponentType, const char*> m_componentNames{};

        // "type string" pointer to a component Array objeect pointer
        std::unordered_map<const char*, std::shared_ptr<I_ComponentArray>> m_componentArrays{};

        // track total components registered
        // this isn't a queue (unlike entities) so component types can't be recycled
        ComponentType m_componentTypeCount = 0;
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
        
        if (m_componentTypeCount >= MAX_COMPONENT_TYPES)
        {
            PLEEPLOG_ERROR("Cannot register component type " + std::string(typeName) + ". Max component count " + std::to_string(MAX_COMPONENT_TYPES) + " exceeded.");
            throw std::runtime_error("ComponentRegistry cannot register component type " + std::string(typeName) + ". Max component count " + std::to_string(MAX_COMPONENT_TYPES) + " exceeded.");
        }

        // add type id/name to next register index
        m_componentTypes.insert({ typeName, m_componentTypeCount });
        m_componentNames.insert({ m_componentTypeCount, typeName });

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

    inline const char* ComponentRegistry::get_component_name(ComponentType componentId)
    {
        if (m_componentNames.find(componentId) == m_componentNames.end())
        {
            PLEEPLOG_ERROR("Cannot retrieve component id " + std::to_string(componentId) + " which has not been registered");
            throw std::range_error("ComponentRegistry cannot retrieve component id " + std::to_string(componentId) + " which has not been registered");
        }

        return m_componentNames[componentId];
    }

    template<typename T>
    void ComponentRegistry::add_component(Entity entity, T component)
    {
        this->_get_component_array<T>()->insert_data_for(entity, component);
    }

    inline void ComponentRegistry::add_component(Entity entity, ComponentType componentId)
    {
        m_componentArrays[get_component_name(componentId)]->emplace_data_for(entity);
    }

    template<typename T>
    void ComponentRegistry::remove_component(Entity entity)
    {
        this->_get_component_array<T>()->remove_data_for(entity);
    }

    inline void ComponentRegistry::remove_component(Entity entity, ComponentType componentId)
    {
        m_componentArrays[get_component_name(componentId)]->clear_data_for(entity);
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
    
    inline void ComponentRegistry::serialize_entity_components(Entity entity, Signature entitySign, EventMessage& msg)
    {
        // Signature size is defined as a ComponentType so no loss is possible
        assert(entitySign.size() == MAX_COMPONENT_TYPES);
        // pack in reverse order, so receiver will read in ascending order
        // ComponentType cannot assign to -1, it will underflow to 255
        for (ComponentType i = static_cast<ComponentType>(entitySign.size()) - 1; i < MAX_COMPONENT_TYPES; i--)
        {
            if (entitySign.test(i))
            {
                // this index is valid
                // get component typename from index (component Id)
                const char* componentTypename = m_componentNames[i];
                //PLEEPLOG_DEBUG("Serializing component: " + std::string(componentTypename) + " into msg(" + std::to_string(msg.size()) + ")");
                m_componentArrays[componentTypename]->serialize_data_for(entity, msg);
            }
        }
    }

    inline void ComponentRegistry::deserialize_entity_components(Entity entity, Signature entitySign, EventMessage& msg)
    {
        for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
        {
            if (entitySign.test(i))
            {
                // TODO: check if component exists locally? 
                // If not we can implicitly add_component a default one (and then it will be updated)

                const char* componentTypename = get_component_name(i);

                // ***If you want to validate components as they are deserialized, do that HERE!
                // stream>> them myself, interrogate their values, and then overwrite the ecs

                // default:
                //PLEEPLOG_DEBUG("Deserializing msg(" + std::to_string(msg.size()) + ") into " + std::string(componentTypename));
                this->deserialize_and_write_component(entity, componentTypename, msg);
            }
        }
    }

    inline void ComponentRegistry::deserialize_and_write_component(Entity entity, const char* componentTypename, EventMessage& msg)
    {
        m_componentArrays[componentTypename]->deserialize_data_for(entity, msg);
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
            throw std::runtime_error("ComponentRegistry cannot get array for component " + std::string(typeName) + " which has not been registered");
        }

        return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]);
    }

    
    inline std::vector<std::string> ComponentRegistry::stringify()
    {
        std::vector<std::string> componentNames;
        componentNames.reserve(m_componentNames.size());
        for (auto componentIt : m_componentNames)
        {
            componentNames.push_back(componentIt.second);
        }

        // Just sort AFTER extracting from the map?
        std::sort(componentNames.begin(), componentNames.end(), 
            [this](std::string a, std::string b)
            {
                return this->m_componentTypes[a.c_str()] < this->m_componentTypes[b.c_str()];
            }
        );
        
        return componentNames;
    }
}

#endif // COMPONENT_REGISTRY_H