#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

//#include "intercession_pch.h"
#include <array>
#include <queue>

#include "ecs/ecs_types.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Manage HostedEntity Ids, and Entity Signatures
    class EntityRegistry
    {
    public:
        EntityRegistry(const TimesliceId localTimesliceIndex = NULL_TIMESLICEID);

        Entity create_entity(bool isTemporal = true);
        bool register_entity(Entity entity);
        size_t destroy_entity(Entity entity);

        // total entities in this Cosmos
        size_t get_entity_count();

        // increase existence count of HostedEntity that this Entity belongs to
        // throws error if count is currently zero (must be set by create)
        void increment_hosted_entity_count(Entity entity);
        // decrease existence count of HostedEntity that this Entity belongs to
        // throws error is count is currently zero (counting as failed)
        void decrement_hosted_entity_count(Entity entity);
        // Returns total number of entities that share this entity's HostedEntityId across the timeline
        // returns 0 if entity is not hosted by this tieslice
        size_t get_hosted_entity_count(Entity entity);
        // Returns total number of TampoeralEntities this cosmos is currently hosting
        size_t get_num_hosted_entities();
        
        void set_signature(Entity entity, Signature sign);

        Signature get_signature(Entity entity);
        
        std::unordered_map<Entity, Signature>& get_signatures_ref();

    private:
        // queue of ALL unused Entity ids
        // Only contains HostedEntityIds with TimesliceId = m_timesliceId
        std::queue<Entity> m_availableHostedEntityIds{};

        // entity signatures for ALL possible entities
        // even those from other host timeslices
        // unlisted entry implies empty signature
        std::unordered_map<Entity, Signature> m_signatures;

        // Known timesliceId to compose Entity values with,
        // and check if we are a server (timeslice host) 
        //     or a client (timeslice guest). Clients will have NULL_TIMESLICEID
        const TimesliceId m_timesliceId = NULL_TIMESLICEID;

        // If we are a timeslice host we arbitrate the registrations of TimesliceId's and
        // CausalChainLinks for entities which originate from our timeslice (we "host" them)
        // Therefore we need to maintain a list of unused ids to register new entities with
        // AND track their instance count across the timeline
        // unindexed values -> count of 0
        // Entities are created at chainlink 0
        // HostedEntities keys MUST ONLY be Entities with CausalChainlink Stripped!
        std::unordered_map<Entity, size_t> m_hostedEntityCounts;
        // NOTE: There may exist multiples of the same link value (because timeslices are async)
        // so count value may not correspond with highest link value
        // TODO: Do we want to be able to detect breaks in the Causal link?
        // It may be needed to trigger an intercession in some cases
    };

    
    inline EntityRegistry::EntityRegistry(const TimesliceId localTimesliceIndex)
        : m_timesliceId(localTimesliceIndex) // clients will have NULL_TIMESLICEID
    {
        // initialize the queue statically with ALL entities (as non are used yet)
        for (GenesisId g = 0; g < GENESISID_SIZE; g++)
        {
            m_availableHostedEntityIds.push(compose_entity(m_timesliceId, g, 0));
        }
    }

    inline Entity EntityRegistry::create_entity(bool isTemporal)
    {
        // assert() entity count doesn't go beyond max
        size_t localEntityCount = m_hostedEntityCounts.size();
        if (localEntityCount > GENESISID_SIZE || m_availableHostedEntityIds.empty())
        {
            PLEEPLOG_ERROR("Cannot exceed max entity capacity of: " + std::to_string(GENESISID_SIZE));
            throw std::range_error("EntityRegistry is at entity count " + std::to_string(localEntityCount) + " and cannot create more Entities.");
        }

        // use queue to get Entity id
        Entity ent = m_availableHostedEntityIds.front();
        assert(m_hostedEntityCounts.find(strip_causal_chain_link(ent)) == m_hostedEntityCounts.end());
        m_availableHostedEntityIds.pop();

        if (!isTemporal) {
            TimesliceId entHostId = derive_timeslice_id(ent);
            GenesisId entGenesisId = derive_genesis_id(ent);
            ent = compose_entity(entHostId, entGenesisId, NULL_CAUSALCHAINLINK);
        }

        // add empty signature
        this->set_signature(ent, Signature{});

        // TODO: how do clients signal to their host to validate this Entity? (and override with a proper TimesliceId)

        // Add ent to hosted map. Set host count to start at 1
        m_hostedEntityCounts[strip_causal_chain_link(ent)] = 1;
        // for consistency we'll internally use the same increment mechanism 
        //     (even though we know who the host is)
        // so we can dispatch to our NetworkDyanmo to do our child's count increment
        //     (event signalled by cosmos after this returns)

        return ent;
    }

    inline bool EntityRegistry::register_entity(Entity entity)
    {
        if (derive_timeslice_id(entity) == m_timesliceId)
        {
            PLEEPLOG_WARN("Registering an Entity " + std::to_string(entity) + " with our host id. Is this intended?");
        }

        // If non empty signature already exists for this entity then something has gone wrong
        assert(m_signatures.find(entity) == m_signatures.end());

        // add empty signature
        this->set_signature(entity, Signature{});

        return true;
    }

    inline size_t EntityRegistry::destroy_entity(Entity entity)
    {
        if (entity >= ENTITY_SIZE)
        {
            PLEEPLOG_ERROR("Cannot destroy entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(ENTITY_SIZE));
            throw std::range_error("EntityRegistry cannot destroy entity " + std::to_string(entity) + " greater than capacity " + std::to_string(ENTITY_SIZE));
        }

        // Clear signature (if it exists)
        const size_t success = m_signatures.erase(entity);

        // Entity will be re-added to availability queue once it's host count decrements to 0
        // we will dispatch to our NetworkDynamo to decrement OTHER hosts
        //     (event signalled by cosmos after this returns)
        // but clients (and servers) will decrement themselves immediately here
        if (success && m_timesliceId == derive_timeslice_id(entity))
        {
            decrement_hosted_entity_count(entity);
        }

        return success;
    }

    inline size_t EntityRegistry::get_entity_count()
    {
        return m_signatures.size();
    }
    
    inline void EntityRegistry::increment_hosted_entity_count(Entity entity)
    {
        CausalChainlink ccl = derive_causal_chain_link(entity);
        UNREFERENCED_PARAMETER(ccl);
        Entity hostedEntity = strip_causal_chain_link(entity);
        auto entityCountsIt = m_hostedEntityCounts.find(hostedEntity);
        if (entityCountsIt == m_hostedEntityCounts.end())
        {
            PLEEPLOG_ERROR("Tried to increment the count of a hosted entity which doesn't exist");
            throw std::range_error("EntityRegistry tried to increment the count of a hosted entity which doesn't exist");
        }
        // count of 0 means decrementer failed to clear entry when it reached 0
        assert(entityCountsIt->second != 0);

        entityCountsIt->second += 1;
        PLEEPLOG_DEBUG("HostedEntity " + std::to_string(hostedEntity) + " host count has incremented to " + std::to_string(entityCountsIt->second) + " from creation of Entity " + std::to_string(entity) + " (link " + std::to_string(ccl) + ")");
    }
    inline void EntityRegistry::decrement_hosted_entity_count(Entity entity)
    {
        CausalChainlink ccl = derive_causal_chain_link(entity);
        UNREFERENCED_PARAMETER(ccl);
        Entity hostedEntity = strip_causal_chain_link(entity);
        auto entityCountsIt = m_hostedEntityCounts.find(hostedEntity);
        if (entityCountsIt == m_hostedEntityCounts.end())
        {
            PLEEPLOG_ERROR("Tried to decrement the count of a hosted entity which doesn't exist");
            throw std::range_error("EntityRegistry tried to decrement the count of a hosted entity which doesn't exist");
        }
        // count of 0 means decrementer failed to clear entry when it reached 0
        assert(entityCountsIt->second != 0);
        entityCountsIt->second -= 1;
        PLEEPLOG_DEBUG("HostedEntity " + std::to_string(hostedEntity) + " host count has decremented to " + std::to_string(entityCountsIt->second) + " from removal of Entity " + std::to_string(entity) + " (link " + std::to_string(ccl) + ")");

        // ensure all counts of 0 become unlisted and re-added to pool
        if (entityCountsIt->second == 0) 
        {
            m_hostedEntityCounts.erase(entityCountsIt);
            m_availableHostedEntityIds.push(hostedEntity);
        }
    }
    inline size_t EntityRegistry::get_hosted_entity_count(Entity entity)
    {
        if (entity == NULL_ENTITY)
        {
            return 0;
        }

        if (derive_timeslice_id(entity) != m_timesliceId)
        {
            PLEEPLOG_WARN("Tried to get host count of HostedEntity which is not hosted by this timeslice");
            return 0;
        }
        
        auto entityCountsIt = m_hostedEntityCounts.find(entity);
        if (entityCountsIt == m_hostedEntityCounts.end())
        {
            return 0;
        }

        return entityCountsIt->second;
    }
    inline size_t EntityRegistry::get_num_hosted_entities()
    {
        return m_hostedEntityCounts.size();
    }
    
    inline void EntityRegistry::set_signature(Entity entity, Signature sign)
    {
        if (entity > ENTITY_SIZE)
        {
            PLEEPLOG_ERROR("Cannot set signature of entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(ENTITY_SIZE));
            throw std::range_error("EntityRegistry set signature of entity " + std::to_string(entity) + " greater than capacity " + std::to_string(ENTITY_SIZE));
        }

        m_signatures[entity] = sign;
    }

    inline Signature EntityRegistry::get_signature(Entity entity)
    {
        if (entity > ENTITY_SIZE)
        {
            PLEEPLOG_ERROR("Cannot get signature of entity " + std::to_string(entity) + " above max capacity of: " + std::to_string(ENTITY_SIZE));
            throw std::range_error("EntityRegistry cannot get signature of entity " + std::to_string(entity) + " greater than capacity " + std::to_string(ENTITY_SIZE));
        }

        try
        {
            return m_signatures.at(entity);
        }
        catch(const std::out_of_range& e)
        {
            // Could be adding first component, so this is expected
            UNREFERENCED_PARAMETER(e);
            //PLEEPLOG_ERROR(e.what());
            //PLEEPLOG_ERROR("No signature exists for Entity: " + std::to_string(entity));
        }
        
        return Signature{};
    }
    
    inline std::unordered_map<Entity, Signature>& EntityRegistry::get_signatures_ref()
    {
        return m_signatures;
    }
}

#endif // ENTITY_REGISTRY_H