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
        // NOTE: This intersect check is for a STATIC collision, and does not
        //   determine the inter-frame time of collision (for continuous resolution)
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

        // ***** Universal collider attributes *****

        // nested transform component "offset" from the local space of entity
        // entities centre is still centre of mass?
        TransformComponent m_offsetTransform;
        
        // ***** collider helpers *****

        // clip clippee polygon against clipper polygon as if they are flattened along axis
        static void pseudo_clip_polyhedra(const std::vector<glm::vec3>& clipper, std::vector<glm::vec3>& clippee, const glm::vec3& axis)
        {
            // this and other must have at least 2 points
            assert(clipper.size() >= 2);
            assert(clippee.size() >= 2);

            // trace along clipper, use each edge to build a plane (combined with axis)
            // determine which side of plane is inside 
            //   (assuming convex shapes we can sample average of all points,
            //    if clipper is size 2 then pass twice, alternating which side is "in")
            // for each clipper plane, trace clippee:
            // if clippee edge crosses plane, add a point for the edge-plane intersection to clipped
            // if clippe edge ends on the "inside" of the plane, add end point to clipped

            // determine centre of clipper
            // given clipper is convex, it will always be the "inside" of each edge plane
            glm::vec3 clipperCentre(0.0f);
            for (size_t i = 0; i < clipper.size(); i++)
            {
                clipperCentre += clipper[i];
            }
            clipperCentre /= clipper.size();

            for (size_t i = 0; i < clipper.size(); i++)
            {
                // avoid clipping along any perpendicular edges
                const glm::vec3 clipperEdgeTangent = glm::cross(axis, clipper[(i+1) % clipper.size()] - clipper[i]);
                if (clipperEdgeTangent == glm::vec3(0.0f))
                {
                    continue;
                }

                // build plane along clipper[i] and [i+1]
                glm::vec4 thisPlane = glm::vec4(
                    glm::normalize(clipperEdgeTangent),
                    0.0f
                );
                // complete plane equation
                thisPlane.w = -1.0f * 
                    (thisPlane.x * clipper[i].x + 
                    thisPlane.y * clipper[i].y + 
                    thisPlane.z * clipper[i].z);
                // determine the "inside" of the plane using clipper centre
                float insideCoeff = 0.0f;
                if (clipper.size() > 2)
                {
                    // but if it is a shape with volume use centre
                    insideCoeff = 
                        thisPlane.x * clipperCentre.x + 
                        thisPlane.y * clipperCentre.y + 
                        thisPlane.z * clipperCentre.z +
                        thisPlane.w;
                }
                // if clipper is just a line then clip one side on first edge pass
                // and clip other side on second edge pass (using a static coeff)
                if (insideCoeff == 0.0f)
                {
                    insideCoeff = INFINITY;
                }

                // to avoid inplace manipulating clipee
                std::vector<glm::vec3> clipped;
                // it may be more efficient to do it inplace? but more complicated

                // maintain previous clippee coeff
                float prevClippeeCoeff = -INFINITY;

                // for first point ONLY check for edge end
                // but return to first point again at the end, ONLY checking for plane crossing
                // all other points check both
                for (size_t j = 0; j < clippee.size() + 1; j++)
                {
                    // if clippee is a line segment (only 2 verticies)
                    // don't check past second point (it is the same edge)
                    if (clippee.size() == 2 && j >= 2)
                    {
                        break;
                    }

                    // get j that accounts for wrapping
                    size_t J = j % clippee.size();
                    // determine which side the point is on
                    float clippeeCoeff = 
                        thisPlane.x * clippee[J].x + 
                        thisPlane.y * clippee[J].y + 
                        thisPlane.z * clippee[J].z +
                        thisPlane.w;

                    // determine case for this edge
                    // buffer for floating point/rounding error, preferring inside
                    const float e = 0.000001f;
                    const bool prevInside = (prevClippeeCoeff >= 0-e && insideCoeff >= 0-e)
                                         || (prevClippeeCoeff <= 0+e && insideCoeff <= 0+e);
                    const bool currInside = (clippeeCoeff     >= 0-e && insideCoeff >= 0-e)
                                         || (clippeeCoeff     <= 0+e && insideCoeff <= 0+e);

                    // we only need inside info, set this for next edge
                    prevClippeeCoeff = clippeeCoeff;

                    // if current is in  & previous is in
                    //      add current
                    // if current is in  & previous is out
                    //      add intersection, and add current
                    // if current is out & previous is in
                    //      add intersection
                    // if current is out & previous is out
                    //      continue

                    // clippee crosses from inside to outside plane or visa-versa (^ -> XOR)
                    //  (don't check intercept on first vertex)
                    if ((currInside ^ prevInside) && j != 0)
                    {
                        // ensure ray is outside -> inside
                        glm::vec3 insideVertex = clippee[(J-1) % clippee.size()];
                        glm::vec3 outsideVertex = clippee[J];
                        if (currInside)
                        {
                            insideVertex = clippee[J];
                            outsideVertex = clippee[(J-1) % clippee.size()];
                        }

                        // find & add intersection
                        glm::vec3 edgeDirection = glm::normalize(insideVertex - outsideVertex);
                        float intersectDist;
                        if (glm::intersectRayPlane(
                            outsideVertex, edgeDirection, 
                            clipper[i], glm::vec3(thisPlane.x, thisPlane.y, thisPlane.z),
                            intersectDist))
                        {
                            clipped.push_back(outsideVertex + edgeDirection*intersectDist);
                        }
                        else
                        {
                            PLEEPLOG_WARN("intersectRayPlane did not find a solution. Coefficient determinant failed or manifolds aren't convex.");
                        }
                    }
                    
                    // after checking crossing, if we land inside add that vertex
                    //  (don't check current on second pass of first index)
                    if (currInside && j < clippee.size())
                    {
                        clipped.push_back(clippee[J]);
                    }
                }

                // set clippee as clipped for next clipper edge
                clippee = clipped;
            }
        }
    };

}

#endif // I_COLLIDER_H