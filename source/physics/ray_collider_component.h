#ifndef RAY_COLLIDER_COMPONENT_H
#define RAY_COLLIDER_COMPONENT_H

//#include "intercession_pch.h"

#include "physics/i_collider_component.h"

namespace pleep
{
    struct RayColliderComponent : public IColliderComponent
    {
        // ***** Ray Specific Attributes *****
        // ray is unit vector (0,0,1), use m_localTransform to rotate and scale ray
        // This would mean that x & y scale have no effect...
        // what if we want an "infinite" length ray?

        // Does not invlude mass or density
        virtual glm::mat3 get_inertia_tensor(glm::vec3 scale = glm::vec3(1.0f)) const override;
        
        virtual const ColliderType get_type() const override
        {
            return ColliderType::ray;
        }
        
        
        // ***** Intersection methods *****
        // Implement dispatches for other collider types
        // Double dispatch other
        virtual bool static_intersect(
            const IColliderComponent* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override;
        
        virtual bool static_intersect(
            const BoxColliderComponent* otherBox, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override;

        // is it useful for rays to collider with other rays?
        virtual bool static_intersect(
            const RayColliderComponent* otherRay, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override;

    private:
        // ***** intersection helper methods *****

        // return the coefficients (lengths) of the interval of a ray collider's projection along an axis
        // DOES NOT compose transform! Expects it to already be composed!
        // always returns [origin,end]
        static glm::vec2 project(const glm::mat4& thisTrans, const glm::vec3& axis);

        // solve ray segment location for parametric variable t
        // we'll allow parameter ouside of [0,1] and leave it at user's risk
        //glm::vec3 solve_parametric(const float t) const;
    };
}

#endif // RAY_COLLIDER_COMPONENT_H