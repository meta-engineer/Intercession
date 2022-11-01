#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

//#include "intercession_pch.h"
#include <array>
#include <queue>

#include "ecs_types.h"
#include "networking/timeline_types.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Manage Entity Ids, Temporal Entity Ids, and causal chain link values for temporal entities
    class EntityRegistry
    {
    public:
        EntityRegistry(const TimesliceId localTimesliceIndex = NULL_TIMESLICE);

        Entity create_local_entity();
        Entity create_temporal_entity();
        Entity register_temporal_entity(TemporalEntity tEntity, CausalChainLink link);
        void destroy_entity(Entity entity);

        // total local entities in this Cosmos
        size_t get_local_entity_count();
        // total unique temporal entities in this Cosmos
        size_t get_temporal_entity_count();

        std::pair<TemporalEntity, CausalChainLink> get_temporal_identifier(Entity entity);
        Entity get_local_entity(TemporalEntity tEntity, CausalChainLink link);

        // throws error if count is currently zero (must be set by create/register)
        void increment_hosted_temporal_entity_count(TemporalEntity tEntity);
        // throws error is count is currently zero (counting as failed)
        void decrement_hosted_temporal_entity_count(TemporalEntity tEntity);
        
        void set_signature(Entity entity, Signature sign);

        Signature get_signature(Entity entity);

    private:
        // queue of ALL unused Entity ids
        std::queue<Entity> m_availableEntities{};

        // static array of entity signatures for ALL entities
        // this is excessive, but at least robust
        // NOTE: std::array does not check bounds
        // TODO: can this be a hashmap? unlisted implies empty signature
        std::array<Signature, MAX_ENTITIES> m_signatures{};

        // runtime tally - used to keep entity limits
        Entity m_entityCount = 0;

        // ***** Temporal Registries *****
        // Besides components the Cosmos also needs to know the connection between 
        // Entities local to only this simulation and Entities shared with the timeline.

        // Known timesliceId to compose TemporalEntity values with,
        // and check if we are a server (timeslice host) or a client (timeslice guess)
        // (clients will have NULL_TIMESLICE)
        const TimesliceId m_timesliceId;

        // If we are a timeslice host we arbitrate the registrations of timeline id's and
        // causal chain links for entities which originate from our timeslice (we "host" them)
        // Therefore we need to maintain a list of unused ids to register new entities with
        // AND track how they map to local entities
        // see "networking/timeline_types.h" for details on TemporalEntity

        // queue of ALL unused TemporalEntity ids (with our host timelineId baked in)
        std::queue<TemporalEntity> m_availableTemporalEntities;

        // map any locally existing Entity in our Cosmos to its temporal id
        std::unordered_map<Entity, std::pair<TemporalEntity, CausalChainLink>>          m_mapLocalToTemporal;
        // reverse 2D map any temporal id to locally existing Entity
        std::unordered_map<TemporalEntity, std::unordered_map<CausalChainLink, Entity>> m_mapTemporalToLocal;

        // Track the hosted TemporalEntities' (originated from this timeslice) 
        // existance count (across the ENTIRE timeline)
        // unindexed values -> count of 0
        std::unordered_map<TemporalEntity, size_t> m_hostEntityCounts;
        // NOTE: There may exist multiples of the same link value (because timeslices are async)
        // so count value may not correspond with highest link value
        // TODO: Do we want to be able to detect breaks in the Causal link?
        // It may be needed to trigger an intercession in some cases
    };

    
    inline EntityRegistry::EntityRegistry(const TimesliceId localTimesliceIndex)
        : m_timesliceId(localTimesliceIndex) // clients will have NULL_TIMESLICE
    {
        // initialize the queue statically with ALL entities (as non are used yet)
        for (Entity e = 0; e < MAX_ENTITIES; e++)
        {
            m_availableEntities.push(e);
        }

        // clients cannot create TemporalEntities
        if (m_timesliceId == NULL_TIMESLICE)
        {
            return;
        }
        // initialize temporal queue with ALL entities, and bake in host id
        for (TemporalEntity hostlessEntityId = 0; hostlessEntityId < MAX_TEMPORAL_ENTITIES; hostlessEntityId++)
        {
            m_availableTemporalEntities.push(compose_temporal_entity(m_timesliceId, hostlessEntityId));
        }
    }

    inline Entity EntityRegistry::create_local_entity()
    {
        if (m_timesliceId != NULL_TIMESLICE)
        {
            PLEEPLOG_WARN("Timeslice host trying to create local entity, if this is a server are you sure you want to do this?");
        }
        // assert() entity count doesn't go beyond max
        if (m_entityCount >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot exceed max entity capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry is at capacity " + std::to_string(MAX_ENTITIES) + " and cannot create more Entities.");
        }

        // use queue to get local id
        Entity ent = m_availableEntities.front();
        m_availableEntities.pop();
        m_entityCount++;

        return ent;
    }

    inline Entity EntityRegistry::create_temporal_entity()
    {
        // clients cannot create TemporalEntities
        if (m_timesliceId == NULL_TIMESLICE)
        {
            PLEEPLOG_WARN("Non timeslice hosts cannot create temporal entities. Implementation required to create local virtual temporal entity");
            return NULL_ENTITY;
        }

        // create local entity as normal
        Entity ent = this->create_local_entity();
        
        // fetch TemporalEntity for ent
        TemporalEntity tEnt = m_availableTemporalEntities.front();
        m_availableTemporalEntities.pop();

        // Check TemporalId (with link 0) is not already used by mistake
        auto temporalIt = m_mapTemporalToLocal.find(tEnt);
        if (temporalIt != m_mapTemporalToLocal.end())
        {
            if (temporalIt->second.find(0) != temporalIt->second.end())
            {
                PLEEPLOG_WARN("Tried to create a new TemporalEntity (" + std::to_string(tEnt) + ") Link (0) which already exists?");
                this->destroy_entity(ent);
                return NULL_ENTITY;
            }
        }

        // set host count to start at 1
        if (!(m_hostEntityCounts.insert({tEnt, 1}).second))
        {
            PLEEPLOG_WARN("Tried to create a new TemporalEntity (" + std::to_string(tEnt) + ") that already has host count " + std::to_string(m_hostEntityCounts[tEnt]) + "?");
            this->destroy_entity(ent);
            return NULL_ENTITY;
        }

        // register in temporal map and reverse temporal map (as link 0)
        m_mapTemporalToLocal.insert({tEnt, std::unordered_map<CausalChainLink, Entity>()})
            .first->second.insert({static_cast<CausalChainLink>(0), ent});
        m_mapLocalToTemporal.insert({ent, std::make_pair(tEnt, static_cast<CausalChainLink>(0))});

        // We init'd count as 1
        // for consistency we'll use the same increment mechanism (even though we know who the host is)
        // so we can dispatch to our NetworkDyanmo to do our child's count increment
        // (event signalled by cosmos after this returns)
    }

    inline Entity EntityRegistry::register_temporal_entity(TemporalEntity tEntity, CausalChainLink link)
    {
        // create local entity as normal
        Entity ent = this->create_local_entity();

        // This entity was passed from another timeslice. Check TemporalId is not already used
        auto temporalIt = m_mapTemporalToLocal.find(tEntity);
        if (temporalIt != m_mapTemporalToLocal.end())
        {
            if (temporalIt->second.find(link) != temporalIt->second.end())
            {
                PLEEPLOG_WARN("Tried to localize a new TemporalEntity (" + std::to_string(tEntity) + ") Link (" + std::to_string(link) + ") which already exists?");
                this->destroy_entity(ent);
                return NULL_ENTITY;
            }
        }

        // do we want to check and exit if the timesliceId is our own?
        if (extract_host_timeslice_id(tEntity) == m_timesliceId)
        {
            PLEEPLOG_WARN("Trying to register a temporal entity that we already host. Is this intended?");
        }

        // register in temporal map and reverse temporal map
        m_mapTemporalToLocal.insert({tEntity, std::unordered_map<CausalChainLink, Entity>()})
            .first->second.insert({link, ent});
        m_mapLocalToTemporal.insert({ent, std::make_pair(tEntity, link)});

        // our parent timeslice already incremented host count when it entered the timestream
        // so we can dispatch to our NetworkDyanmo to do our child's count increment
        // (event signalled by cosmos after this returns)
    }

    inline void EntityRegistry::destroy_entity(Entity entity)
    {
        if (entity >= MAX_ENTITIES)
        {
            PLEEPLOG_ERROR("Cannot destroy entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(MAX_ENTITIES));
            throw std::range_error("EntityRegistry cannot destroy entity " + std::to_string(entity) + " greater than capacity " + std::to_string(MAX_ENTITIES));
        }

        // Clear signature (if it exists)
        m_signatures[entity].reset();

        // re-add entity to queue
        m_availableEntities.push(entity);
        m_entityCount--;

        // temporal entity will be re-added once it's host count decrements to 0

        // remove entity from temporal map and reverse temporal map
        std::pair<TemporalEntity, CausalChainLink> temporalId = get_temporal_identifier(entity);
        m_mapLocalToTemporal.erase(entity);
        m_mapTemporalToLocal[temporalId.first].erase(temporalId.second);
        // if individual TemporalEntity map is now empty, remove it
        if (m_mapTemporalToLocal[temporalId.first].empty()) m_mapTemporalToLocal.erase(temporalId.first);

        // we can dispatch to our NetworkDynamo to do our decrement
        // (event signalled by cosmos after this returns)
        // our child will signal when they receive the remove signal from timestream
    }

    inline size_t EntityRegistry::get_local_entity_count()
    {
        return m_entityCount;
    }
    
    inline size_t EntityRegistry::get_temporal_entity_count()
    {
        return m_mapLocalToTemporal.size();
    }
    
    inline std::pair<TemporalEntity, CausalChainLink> EntityRegistry::get_temporal_identifier(Entity entity)
    {
        auto temporalIt = m_mapLocalToTemporal.find(entity);
        if (temporalIt != m_mapLocalToTemporal.end())
        {
            return temporalIt->second;
        }
        
        return std::make_pair(NULL_TEMPORAL_ENTITY, static_cast<CausalChainLink>(0));
    }

    inline Entity EntityRegistry::get_local_entity(TemporalEntity tEntity, CausalChainLink link)
    {
        auto chainlinkIt = m_mapTemporalToLocal.find(tEntity);
        if (chainlinkIt != m_mapTemporalToLocal.end())
        {
            auto entIt = chainlinkIt->second.find(link);
            if (entIt != chainlinkIt->second.end())
            {
                return entIt->second;
            }
        }

        return NULL_ENTITY;
    }
    
    inline void EntityRegistry::increment_hosted_temporal_entity_count(TemporalEntity tEntity)
    {
        auto entityCountIt = m_hostEntityCounts.find(tEntity);
        if (entityCountIt == m_hostEntityCounts.end())
        {
            PLEEPLOG_ERROR("Tried to increment the count of a hosted entity which doesn't exist");
            throw std::range_error("EntityRegistry tried to increment the count of a hosted entity which doesn't exist");
        }
        assert(entityCountIt->second != 0);
        entityCountIt->second += 1;
        PLEEPLOG_DEBUG("Temporal Entity " + std::to_string(tEntity) + " host count has incremented to " + std::to_string(entityCountIt->second));
    }
    inline void EntityRegistry::decrement_hosted_temporal_entity_count(TemporalEntity tEntity)
    {
        auto entityCountIt = m_hostEntityCounts.find(tEntity);
        if (entityCountIt == m_hostEntityCounts.end())
        {
            PLEEPLOG_ERROR("Tried to decrement the count of a hosted entity which doesn't exist");
            throw std::range_error("EntityRegistry tried to decrement the count of a hosted entity which doesn't exist");
        }
        assert(entityCountIt->second != 0);
        entityCountIt->second -= 1;
        PLEEPLOG_DEBUG("Temporal Entity " + std::to_string(tEntity) + " host count has decremented to " + std::to_string(entityCountIt->second));

        // ensure all counts of 0 become unlisted and re-added to pool
        if (entityCountIt->second == 0) 
        {
            m_hostEntityCounts.erase(entityCountIt);
            m_availableTemporalEntities.push(tEntity);
        }
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