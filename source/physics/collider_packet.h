#ifndef COLLIDER_PACKET_H
#define COLLIDER_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "physics/collider.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    // Forward declare: Relay needs dynamic access to ecs for other components
    class Cosmos;
    
    // ColliderPacket contains reference to a physical collider
    // NOT metadata about a detected collision
    struct ColliderPacket
    {
        TransformComponent& transform;
        // Each packet contains a single collider per entity
        Collider& collider;

        // provide access to ecs for different collision responses
        Entity collidee = NULL_ENTITY;
        std::weak_ptr<Cosmos> owner;
    };
}

#endif // COLLIDER_PACKET_H