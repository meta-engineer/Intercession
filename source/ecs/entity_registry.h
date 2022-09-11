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
        Entity get_entity_count();
        
        void set_signature(Entity entity, Signature sign);

        Signature get_signature(Entity entity);

    private:
        // queue of ALL unused entity IDs
        std::queue<Entity> m_availableEntities{};

        // static array of entity signatures for ALL entities
        // this is excessive, but at least robust
        // NOTE: std::array does not check bounds
        // TODO: can this be a hashmap? unlisted implies empty signature
        std::array<Signature, MAX_ENTITIES> m_signatures{};

        // runtime tally - used to keep entity limits
        Entity m_entityCount{};

        // TODO: Besides the components the Cosmos also needs to know the link between 
        // local Entities and timeline Entities.
        // this data will have to be kept in lockstep with available entities
        //std::unordered_map<Entity, std::pair<TemporalEntity, CausalChainLink>>          m_mapLocalToTimeline;
        //std::unordered_map<TemporalEntity, std::unordered_map<CausalChainLink, Entity>> m_mapTimelineToLocal;

        // who arbitrates the timelineEntityId, and the CausalChainLink?
        // EACH timeslice will arbitrate the entities which start on them (their "host" timeslice)
        // maintaining a list of available ids (like is done for local Entity values)
        // the first 4 bits of TemporalEntity can be the host timesliceId (max 16)
        // (leaving 12 bits -> max 4096 TemporalEntities from each host)

        // The host will also track the Causality Chain for TemportalEntities they host
        // (total count of temporal instances, breaks in Causality Chain, etc...)
        // other timslices can send message to its host to signal creation/deletion

        // Do we only need a total instance count?
        // Or do we need to be able to fetch an entity's causal neighbour?
        // If our link value is X, X-1 should exist, does X+1 exist?
        // I suppose we can infer if count-1 > X then it does.
        // but we can only check that on the host...
        // do we need to be able to fetch other Temporal related Entities in Cosmos? (without host)

        // Race conditions for maintaining Chain counts over async threads:
        // on TemporalEntity registry we set count to 1 (uses link value 0)
        //   and then every Cosmos (including the registering host) who creates an Entity 
        //   of that given TemporalEntity through the normal timestream 
        //   (entities who time-travel dont change overall instance count in the timeline)
        //   will increment count by 1 only if that Cosmos has a child timeslice.
        // This means that some server object (dynamo?) should subscribe to 
        //   some kind of entity creation or timestream events

        
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

    inline Entity EntityRegistry::get_entity_count()
    {
        return m_entityCount;
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