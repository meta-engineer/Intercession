#ifndef BIPED_CONTROL_PACKET_H
#define BIPED_CONTROL_PACKET_H

//#include "intercession_pch.h"
#include "controlling/biped_control_component.h"
#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/ray_collider_component.h"

namespace pleep
{
    struct BipedControlPacket
    {
        BipedControlComponent& controller;
        TransformComponent& transform;
        PhysicsComponent& physics;

        // Biped controller needs a collider with response as "legs"
        // we could enforce that this entity has a ray & spring
        RayColliderComponent& legCollider;

        // need general access to ecs?
    };
}

#endif // BIPED_CONTROL_PACKET_H