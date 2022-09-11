#ifndef TIMELINE_TYPES_H
#define TIMELINE_TYPES_H

//#include "intercession_pch.h"
#include <cassert>

#include "ecs/ecs_types.h"

namespace pleep
{
    // typenames just for code clarity

    // local entities may exceed temporal entities (temporal entities registered by other hosts) and
    // temporal entities may exceed local entities (other hosts using my registered temporal entities)
    using TemporalEntity = Entity; // uint16_t
    // top 4 bits of TemporalEntity are its host timeslice id (1/2 byte)
    // therefore max cannot exceed (Entity type max + 1) >> 4
    const TemporalEntity MAX_TEMPORAL_ENTITIES = 0x10000 >> 4;  // (4096)
    const TemporalEntity MAX_TIMESLICES        = 0x10000 >> 12; // (16)
    inline TemporalEntity extract_host_timeslice_id(TemporalEntity id)
    {
        return id >> 12; // (16-4 bits)
    }
    inline TemporalEntity compose_temporal_entity(TemporalEntity timesliceId, TemporalEntity hostlessEntityId)
    {
        assert(hostlessEntityId < MAX_TEMPORAL_ENTITIES);
        assert(     timesliceId < MAX_TIMESLICES);
        return (timesliceId << 12) + (hostlessEntityId);
    }

    // Represents order of causality for Entities of the same TemporalEntity. Name is kinda wordy
    // Yugioh uses chain link as separate words therefore PascalCase -> ChainLink
    using CausalChainLink = uint8_t;

    // Maybe let's not get too many layers of abstraction deep
    //using TimelineId = std::pair<TemporalEntity, CausalChainLink>;
}

#endif // TIMELINE_TYPES_H