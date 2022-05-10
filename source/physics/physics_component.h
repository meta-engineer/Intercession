#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include "ecs/ecs_types.h"
#include "physics/i_collider_component.h"

namespace pleep
{
    // Having a PhysicsComponent (& TransformComponent) natively enables motion integration
    // Also composing a "Collider" Component (of whatever shape) enableds
    //   triggering collisions
    // Also composing a "Body" component allows for physical responses to other Bodies
    // assigning the response type in the collider component ties the 
    // physics, collider, and body comonents together to provide complete motion dynamics

    // TODO: force generators/gravity/magnetism?
    //   different collider for force field (always a sphere?)
    //   can force effect self?
    //   force field collision resolution? (is this just a trigger?)

    const float INFINITE_MASS = 0;

    // provide attributes to facilitate 3D motion/interaction
    struct PhysicsComponent
    {
        // Motion attributes
        glm::vec3 velocity              = glm::vec3(0.0f);
        glm::vec3 acceleration          = glm::vec3(0.0f);
        glm::vec3 angularVelocity       = glm::vec3(0.0f);  // in RADS!
        glm::vec3 angularAcceleration   = glm::vec3(0.0f);  // in RADS!

        // Drag Factors. 
        // TODO: Should these be determined by the environment (eg. air vs water)
        float linearDrag            = 0.00f;
        float angularDrag           = 0.00f;
        // only applied after collision resolution
        float collisionLinearDrag   = 0.00f;
        float collisionAngularDrag  = 0.03f;

        float mass = 1.0f;
        // TODO: we may want to be able to define some model of non-uniform density
        // and/or generate mass from a collider density & precalculated volume
        //   to avoid poorly configured/"eyeballed" masses
        // if a physics object has more than 1 collider, total collider volume
        //   needs to be taken into account

        // Constraints
        // Because we don't have direct control of entity's transform attributes
        // Using known lock values motion integration relay can "catch" and restore
        // even if other systems don't respect the constraint
        bool      lockOrigin        = false;
        glm::vec3 lockedOrigin      = glm::vec3(0.0f);
        bool      lockOrientation   = false;
        glm::quat lockedOrientation = glm::quat(glm::vec3(0.0f));
        // does entity update velocity/position
        bool isAsleep = false;
    };
}

#endif // PHYSICS_COMPONENT_H