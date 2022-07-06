#include "box_collider_component.h"
#include <array>

#include "logging/pleep_log.h"
#include "physics/ray_collider_component.h"

namespace pleep
{
    const float BoxColliderComponent::manifoldDepth = 0.04f;
    const float BoxColliderComponent::manifoldMinWeight = 0.80f;

    glm::mat3 BoxColliderComponent::get_inertia_tensor(glm::vec3 scale) const
    {
        scale = scale * localTransform.scale;
        // x=width, y=height, z=depth
        // assuming box is a unit cube
        const float width  = scale.x;
        const float height = scale.y;
        const float depth  = scale.z;
        glm::mat3 I(0.0f);
        // Do we have to have adjust dimensions here if we scale the whole
        //   tensor by the combined entity transform after?

        // coefficient of 12 is "real", lower (more resistant) may be needed for stability
        I[0][0] = (height*height +  depth*depth ) / 4.0f;
        I[1][1] = ( width*width  +  depth*depth ) / 4.0f;
        I[2][2] = ( width*width  + height*height) / 4.0f;
        return I;
    }


    bool BoxColliderComponent::static_intersect(
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
    
    bool BoxColliderComponent::static_intersect(
        BoxColliderComponent* otherBox, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint)
    {
        // SAT algorithm (actually Separating Plane Theorum)
        // we can optimize SPT given our polyhedra are rectangular prisms
        // the size of the projected interval of a prism along its own face normal
        // will always be its sidelength (it is always axis aligned to itself)
        // so we can project its centre (origin) and then +/- its sidelength

        // for each face normal (in both shapes) generate all possible axes
        // then use each of those to generate the vec2 interval
        // NOTE: For edge-to-end collision we also need to check 9 cases for 
        // each face normal cross-product with the 3 of the otherBox collider

        // NOTE: angle between 3d vectors:
        // cos(angle) = v1.x*v2.x + v1.y*v2.y + v1.z*v2.z / (|v1| * |v2|);
        // projection of v1 onto v2:
        // v1_proj = cos(angle) * |v1| * norm(v2)

        // entity transform non-uniform scale might not be applicable to certain colliders
        glm::mat4 thisLocalTransform   = this->compose_transform(thisTransform);
        glm::mat3 thisNormalTransform  = glm::transpose(glm::inverse(glm::mat3(thisLocalTransform)));
        glm::mat4 otherLocalTransform  = otherBox->compose_transform(otherTransform);
        glm::mat3 otherNormalTransform = glm::transpose(glm::inverse(glm::mat3(otherLocalTransform)));

        std::array<glm::vec3, 15> axes;
        // each calculated interval must be comparable, so they need to be using
        //   equal sized basis vector (aka normalized)
        // we'll wait until the axes are tested to normalize incase of early exits

        // this' face normals (w=0.0f -> no translation)
        axes[0] = thisNormalTransform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[1] = thisNormalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[2] = thisNormalTransform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        // otherBox's face normals (w=0.0f -> no translation)
        axes[3] = otherNormalTransform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[4] = otherNormalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[5] = otherNormalTransform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        // edge "normals"
        axes[6] = glm::cross(axes[0], axes[3]);
        axes[7] = glm::cross(axes[0], axes[4]);
        axes[8] = glm::cross(axes[0], axes[5]);
        axes[9] = glm::cross(axes[1], axes[3]);
        axes[10]= glm::cross(axes[1], axes[4]);
        axes[11]= glm::cross(axes[1], axes[5]);
        axes[12]= glm::cross(axes[2], axes[3]);
        axes[13]= glm::cross(axes[2], axes[4]);
        axes[14]= glm::cross(axes[2], axes[5]);

        // Maintain data from positive collision tests
        glm::vec3 minPenetrateNormal(0.0f);   // vector of collision force? world-space
        float minPenetrateDepth = INFINITY;   // depth along best normal?

        // continuous collision:
        // get velocity of this relative to otherBox (thisVelocity - otherVelocity)
        // extend the interval of this by that relative velocity

        for (int a = 0; a < 15; a++)
        {
            // wait as late as possible to normalize each axis
            axes[a] = glm::normalize(axes[a]);
            // CHECK! if colliders are alligned cross products might not work!
            if (isnan(axes[a].x))
                continue;

            glm::vec2 intervalA = BoxColliderComponent::project(thisLocalTransform,  axes[a]);
            glm::vec2 intervalB = BoxColliderComponent::project(otherLocalTransform, axes[a]);

            // no continuous collision for now
            // TODO: get relative velocity, extend intervalA by velocity
            // TODO: determine direction of normal based on velocities

            float penetration = 0;
            bool flipAxis = false;
            // determine direction of penetration by midpoints until we account for velocities
            if ((intervalA.x + intervalA.y)/2.0f > (intervalB.x + intervalB.y)/2.0f)
            {
                penetration = intervalB.y - intervalA.x;
            }
            else
            {
                penetration = intervalA.y - intervalB.x;
                // other is ahead of this, so flip axis so that it points "away" from other's surface towards this
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
            }
        }

        // write to references for collision normal, depth (and collision location?)
        collisionNormal = minPenetrateNormal;
        collisionDepth = minPenetrateDepth;

        // use collisionNormal to find penetrating points
        // we could maybe have tracked & maintained these points during projection...
        // Find all points in contact manifold for each object

        std::vector<glm::vec3> thisContactManifold;
        BoxColliderComponent::build_contact_manifold(thisLocalTransform, -collisionNormal, collisionDepth, thisContactManifold);

        std::vector<glm::vec3> otherContactManifold;
        BoxColliderComponent::build_contact_manifold(otherLocalTransform, collisionNormal, collisionDepth, otherContactManifold);

        // Solve for collisionPoint depending on size of manifolds found
        if (otherContactManifold.size() == 0 || thisContactManifold.size() == 0)
        {
            PLEEPLOG_ERROR("Contact Manifold(s) could not be built somehow?!");
            return false;
        }
        else if (otherContactManifold.size() == 1)
        {
            // single penetrating vertex on otherBox
            // prefer using single vertex of otherBox first
            collisionPoint = otherContactManifold.front();
            return true;
        }
        else if (thisContactManifold.size() == 1)
        {
            // single penetrating vertex on this
            // invert to point to be on surface of other
            collisionPoint = thisContactManifold.front() + (collisionNormal * collisionDepth);
            return true;
        }
        else
        {
            //PLEEPLOG_WARN("Non trivial Manifolds... expect something to go wrong!");

            // use this' manifold to clip otherBox's manifold
            I_ColliderComponent::pseudo_clip_polyhedra(thisContactManifold, otherContactManifold, collisionNormal);

            // something went wrong :(
            assert(!otherContactManifold.empty());
            // TODO: for "production" we may want a emergency clause to use simple average if clipping fails

            // now otherBox is completely clipped, use it to find "average" contact point

            // if clipped manifold is only 1 vertex then we can short-cut as the above cases do
            if (otherContactManifold.size() == 1)
            {
                collisionPoint = otherContactManifold.front();
                return true;
            }

            // "Rounding algorithm" of manifold plane
            // we want less deep verticies to contribute less to average
            // assign weights based on their dot product with collisionNormal
            // relative to the max depth dot product

            // generate max coeff from clipped manifold
            // and accumulate average of points at max depth, set that as manifold origin
            float maxCoeff = -INFINITY;
            glm::vec3 manifoldOrigin(0.0f);
            float originContributors = 0;
            for (glm::vec3 v : otherContactManifold)
            {
                const float vertCoeff = glm::dot(v, collisionNormal);
                if (vertCoeff > maxCoeff)
                {
                    // reset max & origin
                    manifoldOrigin = v;
                    originContributors = 1;
                    maxCoeff = vertCoeff;
                }
                else if (vertCoeff == maxCoeff)
                {
                    manifoldOrigin += v;
                    originContributors++;
                }
                // ignore points under current max
            }
            // ensure maxCoeff calculation was valid
            assert(originContributors > 0);
            // safe from divide by zero
            manifoldOrigin /= originContributors;

            // scale otherBox points' weight based on its dot product to collisionNormal
            // linearly scaling out to some min % at a distance of manifoldDepth
            const float manifoldRange = 1.0f - BoxColliderComponent::manifoldMinWeight;
            
            collisionPoint = glm::vec3(0.0f);
            for (glm::vec3 v : otherContactManifold)
            {
                const float vertCoeff = glm::dot(v, collisionNormal);
                const glm::vec3 relativeVertex = v - manifoldOrigin;
                // scale dot product coefficient to range [0 : manifoldDepth],
                // then scale that range to range [0 : 1.0]
                float vertWeight = (vertCoeff-maxCoeff+BoxColliderComponent::manifoldDepth) / BoxColliderComponent::manifoldDepth;
                // make weight non-linear?
                //vertWeight *= vertWeight;
                // then scale that range to range [minWeight : 1.0]
                collisionPoint += relativeVertex * (vertWeight * manifoldRange + BoxColliderComponent::manifoldMinWeight);
            }
            collisionPoint /= otherContactManifold.size();
            collisionPoint += manifoldOrigin;

            return true;
        }

        // we should never get here
        //return false;
    }
    
    bool BoxColliderComponent::static_intersect(
        RayColliderComponent* otherRay, 
        const TransformComponent& thisTransform,
        const TransformComponent& otherTransform,
        glm::vec3& collisionNormal,
        float& collisionDepth,
        glm::vec3& collisionPoint)
    {
        // defer to Ray Collider's implementation
        if (otherRay->static_intersect(this, otherTransform, thisTransform, collisionNormal, collisionDepth, collisionPoint))
        {
            // collision metadata returned is relative to passed this, invert to be relative to other
            collisionNormal *= -1.0f;
            collisionPoint = collisionPoint + (collisionNormal * collisionDepth);
            return true;
        }
        return false;
    }

    
    glm::vec2 BoxColliderComponent::project(const glm::mat4& thisTrans, const glm::vec3& axis)
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
                        CUBE_RADIUS * i,
                        CUBE_RADIUS * j,
                        CUBE_RADIUS * k
                    };

                    vertex = thisTrans * glm::vec4(vertex, 1.0f);

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
    
    void BoxColliderComponent::build_contact_manifold(const glm::mat4& thisTrans, const glm::vec3 axis, const float depth, std::vector<glm::vec3>& dest)
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
                CUBE_RADIUS * dimensions.x,
                CUBE_RADIUS * dimensions.y,
                CUBE_RADIUS * dimensions.z
            };

            // we have to do this matrix multiply again...
            vertex = thisTrans * glm::vec4(vertex, 1.0f);

            const float coeff = glm::dot(vertex, axis);
            maxCoeff = coeff > maxCoeff ? coeff : maxCoeff;

            // We can greedily cull points GUARENTEED to be outside
            // though depending on order recieved this can miss points
            // but it is a *slight* optimization
            if (coeff >= maxCoeff - BoxColliderComponent::manifoldDepth)
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
            if (it->first >= maxCoeff - BoxColliderComponent::manifoldDepth)
            {
                dest.push_back(it->second);
            }
        }
    }

}