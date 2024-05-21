#ifndef COLLISION_PROCEDURES_H
#define COLLISION_PROCEDURES_H

//#include "intercession_pch.h"
#include "physics/collider_packet.h"
#include "physics/physics_component.h"

namespace pleep
{
    // HELPER FUNCTIONS
    
    // unit cube has sidelength of 1.0, sphere has a diameter of 1
    constexpr float UNIT_RADIUS         = 0.5f;

    // callibrations for manifold checking
    // TODO: these may need to be non-const and vary based on size of collider
    // (units are amount of dot product with normal, not actual distance units)
    // depth of contact manifold to captures edges at non-perfect angles
    constexpr float MANIFOLD_DEPTH      = 0.04f;

    // percentage weight of verticies at the max manifold depth
    constexpr float MANIFOLD_MIN_WEIGHT = 0.80f;
    
    // return the coefficents (lengths) of the interval of a box collider's projection along an axis
    // DOES NOT compose transform! Expects it to already be composed!
    // always returns [min,max]
    inline glm::vec2 project_box(const glm::mat4& boxTrans, const glm::vec3& axis)
    {
        // for each (8) vertex 
        //   determine the vertex position
        //   do "dot product" with axis to get cos(angle)
        //   do cos(angle) * |vertex vector| to get coefficient
        //   return smallest and largest coefficients

        float minCoeff = INFINITY;
        float maxCoeff = -INFINITY;

        // incase axis is not normalized
        //const float axisLength = glm::length(axis);

        for (int i = -1; i < 2; i+=2)
        {
            for (int j = -1; j < 2; j+=2)
            {
                for (int k = -1; k < 2; k+=2)
                {
                    glm::vec3 vertex = {
                        UNIT_RADIUS * i,
                        UNIT_RADIUS * j,
                        UNIT_RADIUS * k
                    };

                    vertex = boxTrans * glm::vec4(vertex, 1.0f);

                    //cosAngle = (vertex.x * axis.x + vertex.y * axis.y + vertex.z * axis.z) 
                    //  / (glm::length(vertex) * glm::length(axis));
                    //coeff = cosAngle * glm::length(vertex);
                    // optimized?
                    const float coeff = glm::dot(vertex, axis);

                    minCoeff = std::min(minCoeff, coeff);
                    maxCoeff = std::max(maxCoeff, coeff);
                }
            }
        }
        
        return glm::vec2(minCoeff, maxCoeff);
    }
    
    // return the coefficients (lengths) of the interval of a ray collider's projection along an axis
    // DOES NOT compose transform! Expects it to already be composed!
    // always returns [origin,end]
    inline static glm::vec2 project_ray(const glm::mat4& rayTrans, const glm::vec3& axis)
    {
        // for origin and end
        //   do "dot product" with axis to get cos(angle)
        //   return origin and endpoint coefficients

        const glm::vec3 rayOrigin = rayTrans * glm::vec4(0,0,0, 1.0f);
        const glm::vec3 rayEnd    = rayTrans * glm::vec4(0,0,1, 1.0f);
        
        const float originProjection = glm::dot(rayOrigin, axis);
        const float endProjection    = glm::dot(rayEnd,    axis);

        return glm::vec2(originProjection, endProjection);
    }

    inline static glm::vec2 project_sphere(const glm::mat4& sphereTrans, const glm::vec3& axis)
    {
        // find origin projection
        // then add/subtract radius

        const glm::vec3 sphereOrigin = sphereTrans * glm::vec4(0,0,0, 1.0f);
        // only considers scale along x
        const glm::vec3 sphereSurface = sphereTrans * glm::vec4(UNIT_RADIUS,0,0, 1.0f);

        const float originProjection = glm::dot(sphereOrigin, axis);
        const float radiusProjection = glm::dot(glm::length(sphereSurface - sphereOrigin) * axis, axis);

        return glm::vec2(originProjection - radiusProjection, originProjection + radiusProjection);
    }

    // fill dest with all points on plane perpendicular and farthest along axis
    // Manifold must be returned in winding order around the perimeter
    // uses static manifold calibrations (shared with static_intersect)
    inline void build_contact_manifold(const glm::mat4& thisTrans, const glm::vec3 axis, const float depth, std::vector<glm::vec3>& dest)
    {
        assert(dest.empty());
        // manfold range dependant on collision depth?
        UNREFERENCED_PARAMETER(depth);

        std::vector<std::pair<float, glm::vec3>> allVertices;

        float maxCoeff = -INFINITY;

        // Manifold MUST be built with a winding order!
        // either direction is fine, but it needs to trace the perimeter!
        glm::vec3 dimensions(-1, -1, -1);
        const int order[] = {0,1,0,2,0,1,0,2};
        
        for (int index : order)
        {
            glm::vec3 vertex = {
                UNIT_RADIUS * dimensions.x,
                UNIT_RADIUS * dimensions.y,
                UNIT_RADIUS * dimensions.z
            };

            // we have to do this matrix multiply again...
            vertex = thisTrans * glm::vec4(vertex, 1.0f);

            const float coeff = glm::dot(vertex, axis);
            maxCoeff = coeff > maxCoeff ? coeff : maxCoeff;

            // We can greedily cull points GUARENTEED to be outside
            // though depending on order recieved this can miss points
            // but it is a *slight* optimization
            if (coeff >= maxCoeff - MANIFOLD_DEPTH)
            {
                allVertices.push_back({
                    std::pair<float, glm::vec3>(coeff, vertex)
                });
            }

            // flip dimension at this index in the order
            dimensions[index] = dimensions[index] < 0.0f ? 1.0f : -1.0f;
        }

        // We cannot sort the verticies becuase it can destroy the winding order
        // this means that the farthest vertex in the manifold has no guarenteed index
        
        // read all, pushing within manifold
        for (std::vector<std::pair<float, glm::vec3>>::reverse_iterator it = allVertices.rbegin(); it != allVertices.rend(); ++it)
        {
            if (it->first >= maxCoeff - MANIFOLD_DEPTH)
            {
                dest.push_back(it->second);
            }
        }
    }
    
    // clip clippee polygon against clipper polygon as if they are flattened along axis
    inline void pseudo_clip_polyhedra(const std::vector<glm::vec3>& clipper, std::vector<glm::vec3>& clippee, const glm::vec3& axis)
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
            if (glm::length2(clipperEdgeTangent) == 0.0f)
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

                // get j that accounts for wrapping (must be signed)
                int J = static_cast<int>(j % clippee.size());
                // get previous index (account for wrapping again)
                int J_1 = J==0 ? static_cast<int>(clippee.size()) - 1 : J-1;
                // determine which side the point is on
                float clippeeCoeff = 
                    thisPlane.x * clippee[J].x + 
                    thisPlane.y * clippee[J].y + 
                    thisPlane.z * clippee[J].z +
                    thisPlane.w;

                // determine case for this edge
                // buffer for floating point/rounding error, preferring inside
                const float e = 0.00005f;
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
                    glm::vec3 insideVertex = clippee[J_1];
                    glm::vec3 outsideVertex = clippee[J];
                    if (currInside)
                    {
                        insideVertex = clippee[J];
                        outsideVertex = clippee[J_1];
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


    // INTERSECTION PROCEDURES

    // each intersect procedure should return metadata as follows:
    // collisionPoint will always be on surface of B
    // collisionNormal will always be in the direction of B -> A
    // collisionDepth -> surface of A = collisionPoint - (collisionDepth * collisionNormal)
    // For now specify non-continuous time (static) intersection detection

    bool null_intersect(
        ColliderPacket&,
        ColliderPacket&,
        glm::vec3&, glm::vec3&, float&
    );

    bool box_box_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool box_sphere_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool box_ray_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    
    bool sphere_box_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool sphere_sphere_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool sphere_ray_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );

    bool ray_box_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool ray_sphere_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );
    bool ray_ray_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );


    // PHYSICS RESPONSE PROCEDURES

    void null_response(
        ColliderPacket&, PhysicsComponent&, 
        ColliderPacket&, PhysicsComponent&, 
        glm::vec3&, glm::vec3&, float&
    );

    void rigid_rigid_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );

    void spring_rigid_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );

    void rigid_spring_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    );


    // lookup table for narrow phase collision between different ColliderTypes
    using intersectionProcedure = std::function<
        bool(ColliderPacket&, ColliderPacket&, glm::vec3&, glm::vec3&, float&)
    >;
    static_assert(ColliderType::count == static_cast<ColliderType>(4));
    const intersectionProcedure intersectProcedures[static_cast<size_t>(ColliderType::count)]
                                                   [static_cast<size_t>(ColliderType::count)] = {
        {null_intersect,    null_intersect,         null_intersect,             null_intersect},
        {null_intersect,    box_box_intersect,      box_sphere_intersect,       box_ray_intersect},
        {null_intersect,    sphere_box_intersect,   null_intersect,             null_intersect},
        {null_intersect,    ray_box_intersect,      null_intersect,             ray_ray_intersect}
    };

    // lookup table for collision physics response between different body types
    using responseProcedure = std::function<
        void(ColliderPacket&, PhysicsComponent&, ColliderPacket&, PhysicsComponent&, glm::vec3&, glm::vec3&, float&)
    >;
    static_assert(CollisionType::count == static_cast<CollisionType>(4));
    const responseProcedure responseProcedures[static_cast<size_t>(CollisionType::count)]
                                              [static_cast<size_t>(CollisionType::count)] = {
        {null_response,     null_response,          null_response,              null_response},
        {null_response,     rigid_rigid_response,   rigid_spring_response,      null_response},
        {null_response,     spring_rigid_response,  null_response,              null_response},
        {null_response,     null_response,          null_response,              null_response}
    };

}

#endif // COLLISION_PROCEDURES_H