#ifndef BOX_COLLIDER_COMPONENT_H
#define BOX_COLLIDER_COMPONENT_H

//#include "intercession_pch.h"

#include "physics/i_collider_component.h"

namespace pleep
{
    const float CUBE_RADIUS = 0.5f;

    struct BoxColliderComponent : public IColliderComponent
    {
        // ***** Box specific Attributes *****
        // box is unit cube, use localTransform.scale to set side lengths

    private:
        // callibrations for manifold checking
        // TODO: these may need to be non-const and mutable/unique to different sizes of collider
        // depth of contact manifold to captures edges at non-perfect angles
        //   (in units of dot product with normal, not actual distance units)
        static const float manifoldDepth;
        // percentage weight of verticies at the max manifold depth
        static const float manifoldMinWeight;

    public:
        // "Getter" for inertia tensor, accepts inherited scale
        // Does not include mass or density
        virtual glm::mat3 get_inertia_tensor(glm::vec3 scale = glm::vec3(1.0f)) const override;
        
        virtual const ColliderType get_type() const override
        {
            return ColliderType::box;
        }


        // ***** Intersection methods *****
        // Implement dispatches for other collider types
        // Double dispatch other
        virtual bool static_intersect(
            IColliderComponent* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) override;
        
        virtual bool static_intersect(
            BoxColliderComponent* otherBox, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) override;
        
        virtual bool static_intersect(
            RayColliderComponent* otherRay, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) override;

        // ***** intersection helper methods *****

        // return the coefficents (lengths) of the interval of a box collider's projection along an axis
        // DOES NOT compose transform! Expects it to already be composed!
        // always returns [min,max]
        static glm::vec2 project(const glm::mat4& thisTrans, const glm::vec3& axis);

        // fill dest with all points on plane perpendicular and farthest along axis
        // Manifold must be returned in winding order around the perimeter
        // uses static manifold calibrations (shared with static_intersect)
        static void build_contact_manifold(const glm::mat4& thisTrans, const glm::vec3 axis, const float depth, std::vector<glm::vec3>& dest);
    };
}

#endif // BOX_COLLIDER_COMPONENT_H