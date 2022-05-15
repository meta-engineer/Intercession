#include "ray_collider_component.h"

#include "logging/pleep_log.h"
#include "physics/box_collider_component.h"

namespace pleep
{
    glm::mat3 RayColliderComponent::get_inertia_tensor(glm::vec3 scale) const
    {
        scale = scale * m_localTransform.scale;
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
        const IColliderComponent* other, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint) const
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
        const BoxColliderComponent* otherBox, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint) const
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
        glm::vec2 minBoxInterval(0.0f);       // in lieu of box contact manifold
        glm::vec2 minRayProject(0.0f);        // in lieu of ray contact manifold
        bool flippedAxis = true;              // store object ordering for calculating colliionPoint

        for (int a = 0; a < 3; a++)
        {
            axes[a] = glm::normalize(axes[a]);
            
            glm::vec2 rayProject  = RayColliderComponent::project(thisLocalTransform,  axes[a]);
            glm::vec2 boxInterval = BoxColliderComponent::project(otherLocalTransform, axes[a]);

            float penetration = 0;
            bool flipAxis = false;
            // determine direction of penetration by midpoints until we account for velocities
            if (rayProject.x > (boxInterval.x + boxInterval.y)/2.0f)
            {
                penetration = boxInterval.y - rayProject.y;
            }
            else
            {
                penetration = rayProject.y - boxInterval.x;
                // if box is ahead of ray, flip axis so it points away from box
                flipAxis = true;
            }

            // is there a seperating plane for this axis?
            if (penetration <= 0)
            {
                return false;
            }

            // does this axes have the best overlap?
            if (penetration < minPenetrateDepth)
            {
                minPenetrateDepth = penetration;
                minPenetrateNormal = flipAxis ? -axes[a] : axes[a];
                minBoxInterval = boxInterval;
                minRayProject = rayProject;
                flippedAxis = flipAxis;
            }
        }
        // write to references for collision normal, depth
        collisionNormal = minPenetrateNormal;
        collisionDepth = minPenetrateDepth;

        // TODO: dispatch this to raycollider?
        // recalculate ray points
        glm::vec3 rayOrigin = thisLocalTransform * glm::vec4(0,0,0, 1.0f);
        glm::vec3 rayEnd    = thisLocalTransform * glm::vec4(0,0,1, 1.0f);

        //PLEEPLOG_DEBUG("Interval of Box: " + std::to_string(minBoxInterval.x) + ", " + std::to_string(minBoxInterval.y));
        
        //PLEEPLOG_DEBUG("Interval of Ray (origin,end): " + std::to_string(minRayProject.x) + ", " + std::to_string(minRayProject.y));

        // if origin is inside boxInterval collisionPoint is origin
        if (minBoxInterval.x <= minRayProject.x && minRayProject.x <= minBoxInterval.y)
        {
            collisionPoint = rayOrigin;
            return true;
        }
        // otherwise solve for collision point where ray collides box surface
        else
        {
            float rayParametricCoeff;
            // ray is "above" box interval, use box upper
            if (!flippedAxis)
            {
                rayParametricCoeff = (minBoxInterval.y - minRayProject.x) / (minRayProject.y - minRayProject.x);
            }
            // ray is "below" box interval, use box lower
            else
            {
                rayParametricCoeff = (minBoxInterval.x - minRayProject.x) / (minRayProject.y - minRayProject.x);
            }
            // rayParametric can be outside of bounds if rayOrigin penetrates
            
            // inline solve parametric equation
            //return this->solve_parametric(rayParametricCoeff);
            collisionPoint = rayOrigin + rayParametricCoeff * (rayEnd-rayOrigin);
            //PLEEPLOG_DEBUG("Ray Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
            return true;
        }

        // we should never get here
        //return false;
    }

    // is it useful for rays to collider with other rays?
    bool RayColliderComponent::static_intersect(
        const RayColliderComponent* otherRay, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint) const
    {
        PLEEPLOG_WARN("No implementation for ray-ray collision, skipping...");
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