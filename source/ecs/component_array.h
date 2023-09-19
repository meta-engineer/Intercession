#ifndef COMPONENT_ARRAY_H
#define COMPONENT_ARRAY_H

//#include "intercession_pch.h"
#include <exception>
#include <array>
#include <unordered_map>

#include "ecs/ecs_types.h"
#include "ecs/i_component_array.h"
#include "logging/pleep_log.h"
#include "events/event_types.h"

namespace pleep
{
    // packed array of component template type corresponding to entities (EntityRegistry)
    template<typename T>
    class ComponentArray : public I_ComponentArray
    {
    public:
        // add component to entity
        // 1 to 1 mapping between entity <-> component
        // does NOT overwrite, THROWS if component already exists
	    void insert_data_for(Entity entity, T component);

        // adddefault constructed component to entity
        void emplace_data_for(Entity entity) override;

        // remove component from entity
        // compels strict usage, THROWS if component does not exist
        void remove_data_for(Entity entity);
        
        // safely remove all data for given entity
        void clear_data_for(Entity entity) override;

        // safely check if there is any data for given entity
        bool has_data_for(Entity entity) override;
        
        // Push component data into msg
        // does nothing if component does not exist
        void serialize_data_for(Entity entity, EventMessage& msg) override;
        
        // Pop component data from msg and overwrite;
        // non-strict usage, does nothing if component does not exist
        void deserialize_data_for(Entity entity, EventMessage& msg) override;

        // return reference to component for this entity
        // does NOT return "not found", THROWS if no component exists
        T& get_data_for(Entity entity);

        // linearly find first entity with T equal to component
        // operator == must be defined for T
        Entity find_entity_for(T component);

    private:
        // Goal is to have a PACKED array of components
        // each possible entity has a unique spot available
        std::array<T, ENTITY_SIZE> m_array;

        // Maintained map of entity -> component index
        std::unordered_map<Entity, size_t> m_mapEntityToIndex;

        // Maintained map of component index -> entity
        std::unordered_map<size_t, Entity> m_mapIndexToEntity;

        size_t m_size;
    };
    
    template<typename T>
    void ComponentArray<T>::insert_data_for(Entity entity, T component)
    {
        if (m_mapEntityToIndex.find(entity) != m_mapEntityToIndex.end())
        {
            PLEEPLOG_ERROR("Cannot add component to entity " + std::to_string(entity) + " which already has component of this type");
            throw std::range_error("ComponentArray cannot add component to entity " + std::to_string(entity) + " which already has component of this type");
        }

        // append new entry
        size_t newIndex = m_size;
        m_mapEntityToIndex[entity]   = newIndex;
        m_mapIndexToEntity[newIndex] = entity;
        m_array[newIndex]            = component;
        m_size++;
    }

    template<typename T>
    void ComponentArray<T>::emplace_data_for(Entity entity)
    {
        this->insert_data_for(entity, T{});
    }

    template<typename T>
    void ComponentArray<T>::remove_data_for(Entity entity)
    {
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            PLEEPLOG_ERROR("Cannot remove component from entity " + std::to_string(entity) + " which has no component of this type");
            throw std::range_error("ComponentArray cannot remove component from entity " + std::to_string(entity) + " which has no component of this type");
        }

        // copy end element into removed index
        size_t removedIndex   = m_mapEntityToIndex[entity];
        size_t lastIndex      = m_size - 1;
        m_array[removedIndex] = m_array[lastIndex];

        // Update maps to point to moved index
        Entity lastEntity = m_mapIndexToEntity[lastIndex];
        m_mapEntityToIndex[lastEntity] = removedIndex;
        m_mapIndexToEntity[removedIndex] = lastEntity;

        m_mapEntityToIndex.erase(entity);
        m_mapIndexToEntity.erase(lastIndex);

        m_size--;
    }

    template<typename T>
    T& ComponentArray<T>::get_data_for(Entity entity)
    {
        auto indexIt = m_mapEntityToIndex.find(entity);
        if (indexIt == m_mapEntityToIndex.end())
        {
            PLEEPLOG_ERROR("Cannot retrieve component '" + std::string(typeid(T).name()) + "' from entity " + std::to_string(entity) + " which has no component of this type");
            throw std::range_error("ComponentArray cannot retrieve component '" + std::string(typeid(T).name()) + "' from entity " + std::to_string(entity) + " which has no component of this type");
        }
        // If we found data for NULL_ENTITY, something has gone wrong
        assert(entity != NULL_ENTITY);

        return m_array[indexIt->second];
    }

    template<typename T>
    void ComponentArray<T>::clear_data_for(Entity entity)
    {
        // exit safely if not found
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            return;
        }
        
        this->remove_data_for(entity);
    }
    
    template<typename T>
    bool ComponentArray<T>::has_data_for(Entity entity)
    {
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            return false;
        }
        return true;
    }
    
    template<typename T>
    void ComponentArray<T>::serialize_data_for(Entity entity, EventMessage& msg)
    {
        // exit safely if not found
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            PLEEPLOG_WARN("Tried to serialize component which doesn't exist, ignoring...");
            return;
        }

        msg << this->get_data_for(entity);
    }

    template<typename T>
    void ComponentArray<T>::deserialize_data_for(Entity entity, EventMessage& msg)
    {
        // exit safely if not found
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            PLEEPLOG_WARN("Tried to deserialize to component which doesn't exist, ignoring...");
            return;
        }

        // Assume new msg data is for type T
        T newData;
        msg >> newData;

        // overwrite entity's data with message data
        m_array[m_mapEntityToIndex[entity]] = newData;
    }
    
    template<typename T>
    Entity ComponentArray<T>::find_entity_for(T component)
    {
        for (auto e = m_mapEntityToIndex.begin(); e != m_mapEntityToIndex.end(); e++)
        {
            // == must be defined
            if (m_array[e->second] == component)
            {
                return e->first;
            }
        }
        return NULL_ENTITY;
    }
}

#endif // COMPONENT_ARRAY_H