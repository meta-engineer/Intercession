#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include "ecs/ecs_types.h"
#include "physics/i_collider.h"

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

    // provide attributes to facilitate 3D motion/interaction
    struct PhysicsComponent
    {
        // Motion attributes
        glm::vec3 velocity;
        glm::vec3 acceleration;
        glm::vec3 angularVelocity;      // in RADS! Quaternion?
        glm::vec3 angularAcceleration;  // in RADS! Quaternion?

        // Interaction attributes
        std::shared_ptr<ICollider> collider = nullptr;
        // collider behaviour
        //bool isTrigger = false;
        //Entity onTrigger = NULL_ENTITY;
        // could use a function pointer? finding the script component SHOULD only be O(1)
    };
}

#endif // PHYSICS_COMPONENT_H