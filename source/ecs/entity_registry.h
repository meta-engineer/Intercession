#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

//#include "intercession_pch.h"
#include <array>
#include <queue>

#include "ecs_types.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class EntityRegistry
    {
    public:
        EntityRegistry();

        Entity create_entity();
        void destroy_entity(Entity entity);
        
        void set_signature(Entity entity, Signature sign);

        Signature get_signature(Entity entity);

    private:
        // queue of ALL unused entity IDs
        std::queue<Entity> m_availableEntities{};

        // static array of entity signatures for ALL entities
        // this is excessive, but at least robust
        // NOTE: std::array does not check bounds
        std::array<Signature, MAX_ENTITIES> m_signatures{};

        // runtime tally - used to keep entity limits
        Entity m_entityCount{};
    };

    
    inline EntityRegistry::EntityRegistry()
    {
        // initialize the queue statically with ALL entities (as non are used yet)
        for (Entity e = 0; e < MAX_ENTITIES; e++)
        {
            m_availableEntities.push(e);
        }
    }

    inline Entity EntityRegistry::create_entity()
    {
        // assert()
        if (m_entityCount >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot exceed max entity capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry is at capacity " + std::to_string(MAX_ENTITIES) + " and cannot create more Entities.");
        }

        // use queue to get ID
        Entity ent = m_availableEntities.front();
        m_availableEntities.pop();
        m_entityCount++;

        return ent;
    }

    inline void EntityRegistry::destroy_entity(Entity entity)
    {
        if (entity >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot destroy entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry cannot destroy entity " + std::to_string(entity) + " greater than capacity " + std::to_string(MAX_ENTITIES));
        }

        // Clear signature
        m_signatures[entity].reset();

        // re-add entity to queue
        m_availableEntities.push(entity);
        m_entityCount--;
    }
    
    inline void EntityRegistry::set_signature(Entity entity, Signature sign)
    {
        if (entity >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot set signature of entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry set signature of entity " + std::to_string(entity) + " greater than capacity " + std::to_string(MAX_ENTITIES));
        }

        m_signatures[entity] = sign;
    }

    inline Signature EntityRegistry::get_signature(Entity entity)
    {
        if (entity >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot get signature of entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry cannot get signature of entity " + std::to_string(entity) + " greater than capacity " + std::to_string(MAX_ENTITIES));
        }

        return m_signatures[entity];
    }
}

#endif // ENTITY_REGISTRY_H