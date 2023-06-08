#ifndef ECS_TYPES_H
#define ECS_TYPES_H

//#include "intercession_pch.h"
#include <bitset>
#include <cstdint>
#include <cassert>

namespace pleep
{
    // Top-level constant definitions for the ECS

    // An Entity is a (possibly empty) set of components
    // The Entity value is treated AS an entity
    using Entity = std::uint16_t;
    #define NULL_ENTITY UINT16_MAX  // 65535
    #define ENTITY_SIZE NULL_ENTITY
    
    // Entity values have 3 constituents: TimesliceId, GenesisId and CausalChainLink
    // (Entity constituents are defined with the same bitwidth as Entity for bitmasking)
    
    // A TemporalEntity is the abstract collection of all entities with the same TemporalEntityId
    // A TemporalEntityId is the pair of TimesliceId and GenesisId values.

    // Represents the cosmos (and server) who is "hosting" the entity (its creator)
    // This id is shared with all other entities from that host Cosmos
    // Each cosmos can safely create entities using their timeslice id, knowing it will not collide with any others'
    // Cosmos' will have to track the entities they created, and only free TemporalEntityIds once all Cosmos' have notified that they are no longer using it.
    using TimesliceId = Entity;

    // Identifier for the Entity given by the EntityRegistry of its host Cosmos during creation
    // This value is NOT unique, or exclusive to entities of the same TemporalEntity.
    //     It is not neccesarily the order of genesis, though it may be coincidentally
    using GenesisId = Entity;

    // Represents order of causality for Entities of the same coherency. Name is kinda wordy?
    // Yugioh uses "chain" and "link" as separate words therefore PascalCase -> ChainLink
    using CausalChainlink = Entity;

    // Entity values are represented as a bit-wise composition of:
    //     [1 , 4] (Their host's) TimesliceId (4 bits)
    //     [5 ,12] GenesisId                  (8 bits)
    //     [13,16] CausalChainLink            (4 bits)
    #define TIMESLICEID_MASK       0xF000
    #define GENESISID_MASK         0x0FF0
    #define CAUSALCHAINLINK_MASK   0x000F
    #define TIMESLICEID_OFFSET     12
    #define GENESISID_OFFSET       4
    #define CAUSALCHAINLINK_OFFSET 0
    // Forbid max TimesliceId value to prevent Entity collision with NULL_ENTITY (4 bits -> 15-1 -> 14)
    #define NULL_TIMESLICEID       14
    #define NULL_GENESISID         255
    #define NULL_CAUSALCHAINLINK   15
    #define TIMESLICEID_SIZE       NULL_TIMESLICEID
    #define GENESISID_SIZE         NULL_GENESISID
    #define CAUSALCHAINLINK_SIZE   NULL_CAUSALCHAINLINK
    // There can be 15 timeslices, each timeslice can host 256 entities, each entity can have 16 instances in the timeline


    // Back to regular ECS definitions
    using ComponentType = std::uint8_t; // round up from uint5 (2^5 == 32)
    const ComponentType MAX_COMPONENT_TYPES = 32;
    using Signature = std::bitset<MAX_COMPONENT_TYPES>;

    // We want to be able to differentiate component classes
    // to serialize select groups of components
    // we could build an inheritance structure for all components to use...
    // or we could have the component registry assign a clasification on registry...
    enum class ComponentCategory
    {
        unknown,
        downstream,     // Components which flow out to clients
        upstream,       // Components which flow into servers
        count
    };

    // Helper functions for comprehending Entity bit-wise composition

    inline TimesliceId derive_timeslice_id(Entity e)
    {
        return (e & TIMESLICEID_MASK) >> TIMESLICEID_OFFSET;
    }
    
    inline GenesisId derive_genesis_id(Entity e)
    {
        return (e & GENESISID_MASK) >> GENESISID_OFFSET;
    }

    inline pleep::CausalChainlink derive_causal_chain_link(Entity e)
    {
        return (e & CAUSALCHAINLINK_MASK) >> CAUSALCHAINLINK_OFFSET;
    }

    // Returns the entity from the same TemporalEntity with chainlink 0
    inline Entity strip_causal_chain_link(Entity e)
    {
        return e & (~CAUSALCHAINLINK_MASK);
    }

    inline Entity compose_entity(TimesliceId t, GenesisId g, CausalChainlink c)
    {
        // clients can compose entities using NULL_TIMESLICEID
        assert(t <= NULL_TIMESLICEID);
        assert(g < NULL_GENESISID);
        assert(c < NULL_CAUSALCHAINLINK);
        return (t << TIMESLICEID_OFFSET) | (g << GENESISID_OFFSET) | (c << CAUSALCHAINLINK_OFFSET);
    }
}

#endif // ECS_TYPES_H