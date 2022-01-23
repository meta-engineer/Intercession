#ifndef ECS_TYPES_H
#define ECS_TYPES_H

//#include "intercession_pch.h"
#include <bitset>
#include <cstdint>

namespace pleep
{
    
    // top-level constant definitions for anyone using the ecs

    // These aren't considered ids, the ids ARE entities themselves
    using Entity = std::uint32_t;
    const Entity MAX_ENTITIES = 5000;

    using ComponentType = std::uint8_t; // round up from uint5
    const ComponentType MAX_COMPONENT_TYPES = 32;
    using Signature = std::bitset<MAX_COMPONENT_TYPES>;
}

#endif // ECS_TYPES_H