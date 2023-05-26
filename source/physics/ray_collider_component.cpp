#include "ray_collider_component.h"

#include "logging/pleep_log.h"
#include "physics/box_collider_component.h"

namespace pleep
{
    glm::mat3 RayColliderComponent::get_inertia_tensor(glm::vec3 scale) const
    {
        scale = scale * localTransform.scale;
        glm::mat3 I(0.0f);

        // assumes rod is along z axis
        // coefficient of 3 is "real", lower (more resistant) may be needed for stability
        I[0][0] = (scale.z*scale.z) / 3.0f;
        I[1][1] = (scale.z*scale.z) / 3.0f;
        // we need to pretend the ray is a cylinder with some non-zero radius
        I[2][2] = (0.0001f);
        return I;
    }
    
    bool RayColliderComponent::static_intersect(
        I_ColliderComponent* other, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint)
    {
        if (other->static_intersect(this, otherTransform, thisTransform, collisionNormal, collisionDepth, collisionPoint))
        {
            // collision metadata returned is relative to passed this, invert to be relative to other
            collisionNormal *= -1.0f;
            collisionPoint = collisionPoint + (collisionNormal * collisionDepth);
            return true;
        }
        return false;
    }
    
    bool RayColliderComponent::static_intersect(
        BoxColliderComponent* otherBox, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint)
    {
        // Perform SAT only on the box's axes
        
        glm::mat4 thisLocalTransform   = this->compose_transform(thisTransform);
        glm::mat3 thisNormalTransform  = glm::transpose(glm::inverse(glm::mat3(thisLocalTransform)));
        glm::mat4 otherLocalTransform  = otherBox->compose_transform(otherTransform);
        glm::mat3 otherNormalTransform = glm::transpose(glm::inverse(glm::mat3(otherLocalTransform)));

        std::array<glm::vec3, 3> axes;
        axes[0] = otherNormalTransform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[1] = otherNormalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[2] = otherNormalTransform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        
        // Maintain data from positive collision tests
        glm::vec3 minPenetrateNormal(0.0f);   // vector of collision force? world-space
        float minPenetrateDepth = INFINITY;   // depth along best normal?
        float maxRayParametricValue = 0.0f;   // in lieu of contact manifold

        for (int a = 0; a < 3; a++)
        {
            axes[a] = glm::normalize(axes[a]);
            
            glm::vec2 rayProject  = RayColliderComponent::project(thisLocalTransform,  axes[a]);
            glm::vec2 boxInterval = BoxColliderComponent::project(otherLocalTransform, axes[a]);

            float penetration = 0;
            float rayParametricValue = 0;
            bool flipAxis = false;

            // determine direction of penetration with ray origin until we account for velocities
            if (rayProject.x < (boxInterval.x + boxInterval.y)/2.0f)
            {
                // if box is ahead of ray, flip axis so it points away from box
                flipAxis = true;
            }

            // if ray origin is inside of box then rayParameter is 0 and penetration is: 
            //      if flipAxis==false min(ray origin, ray end) -> box top
            //      if flipAxis== true box bottom -> max(ray origin, ray end)
            // if ray origin is outside of box (but ray end is inside) then:
            //      if flipAxis==false rayParameter = max(box top, ray end) -> ray origin
            //                      & penetration = ray end -> box top (negative means no collision)
            //      if flipAxis== true rayParameter = ray origin -> min(box bottom, ray end)
            //                      & penetration = box bottom -> ray end (negative means no collision)

            // origin is inside
            if (rayProject.x >= boxInterval.x && rayProject.x <= boxInterval.y)
            {
                rayParametricValue = 0;
                if (!flipAxis)
                {
                    penetration = boxInterval.y - glm::min(rayProject.x, rayProject.y);
                }
                else
                {
                    penetration = glm::max(rayProject.x, rayProject.y) - boxInterval.x;
                }

                // collision must always happen
                //assert(penetration >= 0);
                if (penetration < 0)
                {
                    PLEEPLOG_WARN("Negative penetration for enclosed ray origin");
                    return false;
                }
            }
            // origin is outside
            else
            {
                if (!flipAxis)
                {
                    const float rayRange = rayProject.x - glm::max(boxInterval.y, rayProject.y);
                    const float rayPointDelta = (rayProject.x - rayProject.y);
                    if (rayPointDelta != 0.0f)
                        rayParametricValue = rayRange / rayPointDelta;
                    penetration = boxInterval.y - rayProject.y;
                }
                else
                {
                    const float rayRange = glm::min(boxInterval.x, rayProject.y) - rayProject.x;
                    const float rayPointDelta = (rayProject.y - rayProject.x);
                    if (rayPointDelta != 0.0f)
                        rayParametricValue = rayRange / rayPointDelta;
                    penetration = rayProject.y - boxInterval.x;
                }

                // neither origin OR end are within box interval -> seperating plane for this axis
                if (penetration < 0)
                    return false;
            }

            // does this axes have the best overlap? max ray parameter -> min penetration (like usual SAT)
            if (rayParametricValue >= maxRayParametricValue)
            {
                maxRayParametricValue = rayParametricValue;
                minPenetrateDepth = penetration;
                minPenetrateNormal = flipAxis ? -axes[a] : axes[a];
            }
        }
        // write to references for collision normal, depth
        collisionNormal = minPenetrateNormal;
        collisionDepth = minPenetrateDepth;

        // parametric value must always be valid
        //assert(maxRayParametricValue >= 0 && maxRayParametricValue <= 1);
        if (maxRayParametricValue < 0 || maxRayParametricValue > 1)
        {
            PLEEPLOG_WARN("Ray parametric value outside of possible range [0,1]");
            return false;
        }

        // remember closest parametric value this physics step to avoid double collision
        if (maxRayParametricValue >= this->minParametricValue)
        {
            return false;
        }
        this->minParametricValue = maxRayParametricValue;

        // TODO: dispatch this to raycollider?
        //return this->solve_parametric(rayParametricValue);

        // inline solve parametric equation
        // recalculate ray points
        glm::vec3 rayOrigin = thisLocalTransform * glm::vec4(0,0,0, 1.0f);
        glm::vec3 rayEnd    = thisLocalTransform * glm::vec4(0,0,1, 1.0f);
        collisionPoint = rayOrigin + maxRayParametricValue * (rayEnd-rayOrigin);

        //PLEEPLOG_DEBUG("Ray Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
        return true;
    }

    // is it useful for rays to collider with other rays?
    bool RayColliderComponent::static_intersect(
        RayColliderComponent* otherRay, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint)
    {
        //PLEEPLOG_WARN("No implementation for ray-ray collision, skipping...");
        UNREFERENCED_PARAMETER(otherRay);
        UNREFERENCED_PARAMETER(thisTransform);
        UNREFERENCED_PARAMETER(otherTransform);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
        return false;
    }
    
    glm::vec2 RayColliderComponent::project(const glm::mat4& thisTrans, const glm::vec3& axis)
    {
        // for origin and end
        //   do "dot product" with axis to get cos(angle)
        //   return origin and endpoint coefficients

        const glm::vec3 rayOrigin = thisTrans * glm::vec4(0,0,0, 1.0f);
        const glm::vec3 rayEnd    = thisTrans * glm::vec4(0,0,1, 1.0f);
        
        const float originProjection = glm::dot(rayOrigin, axis);
        const float endProjection    = glm::dot(rayEnd,    axis);

        return glm::vec2(originProjection, endProjection);
    }
}