#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include "ecs/ecs_types.h"

namespace pleep
{
    // provide attributes to facilitate 3D motion/interaction
    struct PhysicsComponent
    {
        // Motion attributes
        glm::vec3 velocity;
        glm::vec3 acceleration;
        glm::vec3 angularVelocity;      // in RADS! Quaternion?
        glm::vec3 angularAcceleration;  // in RADS! Quaternion?

        // Interaction attributes
        //std::shared_ptr<ICollider> collider;
        bool isTrigger = false;
        // this could be a function pointer, finding the component SHOULD be O(1)
        Entity onTrigger = NULL_ENTITY;

        // possible properties of a physics enabled object:
        // motion updates?
        //   does/doesn't apply velocities
        //   maximum velocities?
        //   motion integration technique? scaling with velocity?
        // collision detection?
        //   triggers on collision? (pass collider and collidee to script?)
        //     collideStart trigger?
        //     collideUpdate trigger?
        //   rigid body collision resolution?
        //   force field collision resolution? (is this just a trigger?)
        // force generators?
        //   different collider for force field (always a sphere?)
        //   can force effect self?
    };
}

#endif // PHYSICS_COMPONENT_H