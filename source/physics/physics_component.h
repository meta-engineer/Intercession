#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include "ecs/ecs_types.h"
#include "physics/i_collider_component.h"

namespace pleep
{
    // possible properties of a physics enabled object:
    // motion updates?
    //   does/doesn't apply velocities
    //   maximum velocities?
    //   motion integration technique? scaling with velocity?
    // collision detection?
    //   multiple colliders? (composite)
    //   triggers on collision? (pass collider and collidee to script?)
    //     collideStart trigger?
    //     collideUpdate trigger?
    //   rigid body collision resolution?
    //   force field collision resolution? (is this just a trigger?)
    // force generators?
    //   different collider for force field (always a sphere?)
    //   can force effect self?

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
        // TODO: Can these be determined by the environment (eg. air vs water)
        float linearDrag            = 0.00f;
        float angularDrag           = 0.00f;
        // only applied after collision resolution
        float collisionLinearDrag   = 0.00f;
        float collisionAngularDrag  = 0.03f;

        // Material attributes
        // we may want to be able to define some model of non-uniform density
        // and/or generate mass from a collider density & precalculated volume
        //   to avoid poorly configured/"eyeballed" masses
        // TODO: if a physics object has more than 1 collider, these attributes
        //   may have to be defined per collider
        float mass = 1.0f;
        //float restitution          = 0.50f;
        //float staticFriction       = 0.40f;
        //float dynamicFrictionCoeff = 0.30f;

        // Constraints
        bool lockOrigin             = false;
        glm::vec3 lockedOrigin      = glm::vec3(0.0f);
        bool lockOrientation        = false;
        glm::quat lockedOrientation = glm::quat(glm::vec3(0.0f));
        // does entity update velocity/position
        bool isAsleep = false;
    };
}

#endif // PHYSICS_COMPONENT_H