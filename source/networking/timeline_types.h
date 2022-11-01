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
    const TemporalEntity NULL_TEMPORAL_ENTITY = NULL_ENTITY;
    // must be same bitwidth as TemporalEntity for bitshifting
    using TimesliceId = TemporalEntity;
    const TimesliceId NULL_TIMESLICE = NULL_TEMPORAL_ENTITY;
    // top 4 bits of TemporalEntity are its host timeslice id (1/2 byte)
    // therefore max cannot exceed (type size) >> 4
    const TemporalEntity MAX_TEMPORAL_ENTITIES = (0xFFFF + 1) >> 4;  // (4096)
    // and TimesliceId cannot exceed 4 bits
    const TimesliceId MAX_TIMESLICES        = (0xF + 1);         // (16)
    
    inline TimesliceId extract_host_timeslice_id(TemporalEntity id)
    {
        return id >> 12; // (16-4 bits)
    }
    inline TemporalEntity compose_temporal_entity(TimesliceId timesliceId, TemporalEntity hostlessEntityId)
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