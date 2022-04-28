#ifndef PHYSICS_CONTROL_PACKET_H
#define PHYSICS_CONTROL_PACKET_H

//#include "intercession_pch.h"
#include "controlling/physics_control_component.h"
#include "physics/transform_component.h"
#include "physics/physics_component.h"

namespace pleep
{
    struct PhysicsControlPacket
    {
        PhysicsControlComponent& controller;
        TransformComponent& transform;
        PhysicsComponent& physics;
    };
}

#endif // PHYSICS_CONTROL_PACKET_H