#ifndef PHYSICS_CONTROL_COMPONENT_H
#define PHYSICS_CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#include "controlling/a_control_component.h"

namespace pleep
{
    // "physics based" controllers can come in many flavours
    enum PhysicsControlType
    {
        position,
        velocity,
        accelleration,
    };

    struct PhysicsControlComponent : public A_ControlComponent
    {
        // ***** physics controller specific attributes *****
        float acceleration    = 5.0f;
        float maxVelocity     = 10.0f;

        // designate desired relay type dispatched by dynamo
        PhysicsControlType type = PhysicsControlType::position;
    };
}

#endif // PHYSICS_CONTROL_COMPONENT_H