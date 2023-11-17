#ifndef BIPED_COMPONENT_H
#define BIPED_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/gtx/quaternion.hpp>

namespace pleep
{
    // storage component used by biped behaviours
    struct BipedComponent
    {
        // this property may have to align with net world forces (gravity/bouyancy/mangnetism?)
        //   but not acceleration due to collision
        // it may also have to influence collider/entity orientation
        glm::vec3 supportAxis = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::quat aimOrientation = glm::quat(glm::vec3(0.0f));
        float horizontalRotationFactor = 1.2f;
        float verticalRotationFactor   = 1.0f;
        float rotationGimbalLimit      = 0.1f;

        // State-machine to be read by animation system
        //BipedState state;

        bool isGrounded = true;
        glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        // may have to recalculate collisionPoint relative velocity
        glm::vec3 groundVelocity = glm::vec3(0.0f);

        // to avoid doubling inputs if client updates lag, these actions have to be robust to bad input
        double jumpCooldownTime = 0.3; // if fixed update is 60hz, this means 60 frames
        double jumpCooldownRemaining = 0.0;
        float jumpForce = 20000.0f;

        double shootCooldownTime = 1.0;
        double shootCooldownRemaining = 0.0;
        float shootForce = 20.0f;

        // units roughly equate to meters and seconds
        float groundAcceleration    = 20.0f;
        //float groundDeceleration    = 40.0f; // is this additive or replacing normal acceleration?
        float groundMaxSpeed        = 8.0f;  // non-controller forces can accelerate faster

        float airAcceleration       = 1.0f;
        //float airDeceleration       = 1.0f;
        float airMaxSpeed           = 4.0f;
    };
}

#endif // BIPED_COMPONENT_H