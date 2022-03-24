#ifndef I_COLLIDER_H
#define I_COLLIDER_H

//#include "intercession_pch.h"
#include <memory>
#include "logging/pleep_log.h"
#include "physics/transform_component.h"

namespace pleep
{
    // we want a singular class pointer to store in component,
    // but we also need specific data individual to each type,
    // intersect method needs to double dispatch both colliders

    enum ColliderType
    {
        none,
        //AABB,
        box,
        //sphere,
        //mesh
    };

    // Forward declare all class types to dispatch to
    //class AABBCollider;
    class BoxCollider;
    //class SphereCollider;
    //class MeshCollider;

    class ICollider
    {
    public:
        // type should be none if subclass forgets to define it
        ICollider(const ColliderType thisType = ColliderType::none)
            : type(thisType)
        {}
        // immutable type for identifying ICollider pointer
        const ColliderType type;

        // Derived colliders should Double-Dispatch on other->intersects
        virtual bool intersects(
            const ICollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth) const = 0;

        virtual bool intersects(
            const BoxCollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth) const
        {
            PLEEPLOG_WARN("No implementation for collision between type (?) and BoxCollider");
            UNREFERENCED_PARAMETER(other);
            UNREFERENCED_PARAMETER(thisTransform);
            UNREFERENCED_PARAMETER(otherTransform);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            return false;
        }
        
    };
}

#endif // I_COLLIDER_H