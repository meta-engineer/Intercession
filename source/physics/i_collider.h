#ifndef I_COLLIDER_H
#define I_COLLIDER_H

//#include "intercession_pch.h"
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

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
        // collisionNormal will always be in direction other -> this
        virtual bool intersects(
            const ICollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const = 0;

        virtual bool intersects(
            const BoxCollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const
        {
            PLEEPLOG_WARN("No implementation for collision between type (?) and BoxCollider");
            UNREFERENCED_PARAMETER(other);
            UNREFERENCED_PARAMETER(thisTransform);
            UNREFERENCED_PARAMETER(otherTransform);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
            return false;
        }

        // Does not include mass or density
        virtual glm::mat3 getInertiaTensor() const
        {
            // we'll use a unit sphere as default
            return glm::mat3(2.0f/5.0f);
        }
        
        
        // ***** collider helpers *****

        // clip clipper polygon against clipper polygon as if they are flattenned along axis
        static void pseudo_clip_polyhedra(std::vector<glm::vec3>& clipper, std::vector<glm::vec3>& clippee, const glm::vec3& axis)
        {
            assert(clipper.size() >= 2);
            assert(clippee.size() >= 2);

            // this and other have at least 2 points,
            // clip other manifold by this manifold completely, then average the points
            for (size_t i = 0; i < clipper.size(); i++)
            {
                // build plane along thisContactManifold[i] and [i+1]
                const glm::vec3 thisPlaneNormal = glm::normalize(glm::cross(axis, clipper[(i+1) % clipper.size()] - clipper[i]));
                // determine the "in"-side of the plane with another polygon point
                // if this is just a line then always clip
                //float insideCoeff;

                // maintain other's previous coeff
                //float prevOtherCoeff = 0;

                // go through all points in other and clip them to this plane
                for (size_t j = 0; i < clippee.size(); j++)
                {
                    // determine which side the point is on

                    // if current point is in and prev was out
                }

                // if one point of other is in and one is out, clip out to the plane

                //if (glm::intersectRayPlane(clippee[0], clippee[1] - clippee[0], clipper[i], thisPlaneNormal, ...))
                
            }
        }
    };

}

#endif // I_COLLIDER_H