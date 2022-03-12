#ifndef PHYSICS_PACKET_H
#define PHYSICS_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "physics/physics_component.h"

namespace pleep
{
    // PhysicsPacket should contain all mutable objects during motion integration
    struct PhysicsPacket
    {
        TransformComponent& transform;
        PhysicsComponent& physics;
    };
}

#endif // PHYSICS_PACKET_H