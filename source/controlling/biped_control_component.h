#ifndef BIPED_CONTROL_COMPONENT_H
#define BIPED_CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/gtx/quaternion.hpp>
#include "controlling/i_control_component.h"

namespace pleep
{
    // Controller component for "characters"
    // Biped implies they have (approximately) one contact point with the ground
    // and can rotate about their upright axis without complex limb movement
    // also may include properties which imply arms (like mantling/climbing)

    // Biped needs to have components: transform, physics
    struct BipedControlComponent : public IControlComponent
    {
        // this property may have to align with net world forces (gravity/bouyancy/mangnetism?)
        //   but not acceleration due to contact
        // it may also have to inform collider/entity orientation
        glm::vec3 supportAxis = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::quat aimOrientation = glm::quat(glm::vec3(0.0f));
        float horizontalRotationFactor = 1.2f;
        float verticalRotationFactor   = 1.0f;
        float rotationGimbalLimit      = 0.1f;

        // State-machine to be output to animation system
        //BipedState state;

        bool isGrounded = true;
        // given as closest (dot product) vector to Transform heading along the ground collider
        glm::vec3 lastGroundGradient = glm::vec3(0.0f, 0.0f, 1.0f);
        // may have to recalculate collisionPoint relative velocity
        glm::vec3 groundVelocity = glm::vec3(0.0f);

        // units roughly equate to meters and seconds
        float groundAcceleration    = 3.0f;
        float groundDeceleration    = 6.0f; // is this additive or replacing normal acceleration?
        float groundMaxSpeed        = 9.0f; // non-controller forces can accelerate faster

        float airAcceleration       = 1.0f;
        //float airDeceleration       = 1.0f;
        float airMaxSpeed           = 20.0f;
    };
}

#endif // BIPED_CONTROL_COMPONENT_H