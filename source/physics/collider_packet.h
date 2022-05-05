#ifndef COLLIDER_PACKET_H
#define COLLIDER_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "physics/i_collider_component.h"
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
        // dynamo/relay can read collider->type from base pointer
        IColliderComponent* collider;

        // provide access to ecs for different collision responses
        Entity collidee;
        Cosmos* owner;
    };
}

#endif // COLLIDER_PACKET_H