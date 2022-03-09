#ifndef COMPONENT_ARRAY_H
#define COMPONENT_ARRAY_H

//#include "intercession_pch.h"
#include <exception>
#include <array>
#include <unordered_map>

#include "ecs_types.h"
#include "i_component_array.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // packed array of component template type corresponding to entities (EntityRegistry)
    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        // add component to entity
        // 1 to 1 mapping between entity <-> component
        // does NOT overwrite, THROWS if component already exists
	    void insert_data_for(Entity entity, T component);

        // remove component from entity
        // compels strict usage, THROWS if component does not exist
        void remove_data_for(Entity entity);
        
        // safely remove all data for given entity
        void clear_data_for(Entity entity) override;

        // return reference to component for this entity
        // does NOT return "not found", THROWS if no component exists
        T& get_data_for(Entity entity);

        // find first entity with T equal to component
        // operator == must be defined for T
        Entity find_entity_for(T component);

    private:
        // Goal is to have a PACKED array of components
        // each possible entity has a unique spot available
        std::array<T, MAX_ENTITIES> m_array;

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
    void ComponentArray<T>::remove_data_for(Entity entity)
    {
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            PLEEPLOG_ERROR("Cannot remove component from entity " + std::to_string(entity) + " which has no component of this type");
            throw std::range_error("ComponentArray cannot remove component from entity " + std::to_string(entity) + " which has no component of this type");
        }

        // copy end element into removed index
        size_t removedIndex = m_mapEntityToIndex[entity];
        size_t lastIndex    = m_size - 1;

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
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            PLEEPLOG_ERROR("Cannot retrieve component from entity " + std::to_string(entity) + " which has no component of this type");
            throw std::range_error("ComponentArray cannot retrieve component from entity " + std::to_string(entity) + " which has no component of this type");
        }

        return m_array[m_mapEntityToIndex[entity]];
    }

    template<typename T>
    void ComponentArray<T>::clear_data_for(Entity entity)
    {
        if (m_mapEntityToIndex.find(entity) == m_mapEntityToIndex.end())
        {
            this->remove_data_for(entity);
        }
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