#ifndef BEHAVIORS_PACKET_H
#define BEHAVIORS_PACKET_H

//#include "intercession_pch.h"

#include "behaviors/behaviors_component.h"
#include "ecs/ecs_types.h"
#include "core/cosmos.h"

namespace pleep
{
    struct BehaviorsPacket
    {
        BehaviorsComponent& behaviors;

        // Provide Cosmos-wide access
        Entity entity = NULL_ENTITY;
        std::weak_ptr<Cosmos> owner;
    };
}

#endif // BEHAVIORS_PACKET_H