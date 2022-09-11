#ifndef PHYSICS_CONTROL_COMPONENT_H
#define PHYSICS_CONTROL_COMPONENT_H

//#include "intercession_pch.h"

namespace pleep
{
    // "physics based" controllers can come in many flavours
    enum PhysicsControlType
    {
        position,
        velocity,
        accelleration,
    };

    struct PhysicsControlComponent
    {
        // ***** physics controller specific attributes *****
        float acceleration    = 5.0f;
        float maxVelocity     = 10.0f;

        // designate desired relay type dispatched by dynamo
        PhysicsControlType type = PhysicsControlType::position;
    };
}

#endif // PHYSICS_CONTROL_COMPONENT_H