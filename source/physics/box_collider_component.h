#ifndef BOX_COLLIDER_COMPONENT_H
#define BOX_COLLIDER_COMPONENT_H

//#include "intercession_pch.h"
#include <string>
#include <array>

#include "physics/i_collider_component.h"
#include "logging/pleep_log.h"

namespace pleep
{
    struct BoxColliderComponent : public IColliderComponent
    {
        // dimensions default 0.5i, 0.5j, 0.5k (aka. all sidelengths 1.0)
        BoxColliderComponent(glm::vec3 dimensions = glm::vec3(0.5f))
        {
            m_dimensions = dimensions;
        }
        
        // ***** Box specific Attributes *****
        // distance each face is from origin (in local space) like a radius
        // opposite faces will be uniform distance from entity origin
        glm::vec3 m_dimensions;

    private:
        // callibrations for manifold checking
        // depth of contact manifold to captures edges at non-perfect angles
        //   (in units of dot product with normal, not actual distance units)
        float m_manifoldDepth = 0.04f;
        // percentage weight of verticies at the max manifold depth
        float m_manifoldMinWeight = 0.80f;

    public:
        // Does not include mass or density
        inline virtual glm::mat3 get_inertia_tensor() const override
        {
            glm::mat3 I(0.0f);
            // x=width, y=height, z=depth
            // coefficient of 12 is "real", lower (more resistant) may be needed for stability
            I[0][0] = (m_dimensions.y*m_dimensions.y + m_dimensions.z*m_dimensions.z)/1.0f;
            I[1][1] = (m_dimensions.x*m_dimensions.x + m_dimensions.z*m_dimensions.z)/1.0f;
            I[2][2] = (m_dimensions.x*m_dimensions.x + m_dimensions.y*m_dimensions.y)/1.0f;
            return I;
        }
        
        const ColliderType get_type() const override
        {
            return ColliderType::box;
        }


        // ***** Intersection methods *****
        // Implement dispatches for other collider types
        // Double dispatch other
        bool static_intersect(
            const IColliderComponent* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override
        {
            if (other->static_intersect(this, otherTransform, thisTransform, collisionNormal, collisionDepth, collisionPoint))
            {
                // any integral return values should be inverted due to double dispatch
                collisionNormal *= -1.0f;
                return true;
            }
            return false;
        }
        
        bool static_intersect(
            const BoxColliderComponent* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override
        {
            // SAT algorithm (actually Separating Plane Theorum)
            // we can optimize SPT given our polyhedra are rectangular prisms
            // the size of the projected interval of a prism along its own face normal
            // will always be its sidelength (it is always axis aligned to itself)
            // so we can project its centre (origin) and then +/- its sidelength

            // for each face normal (in both shapes) generate all possible axes
            // then use each of those to generate the vec2 interval
            // NOTE: For edge-to-end collision we also need to check 9 cases for 
            // each face normal cross-product with the 3 of the other collider

            // NOTE: angle between 3d vectors:
            // cos(angle) = v1.x*v2.x + v1.y*v2.y + v1.z*v2.z / (|v1| * |v2|);
            // projection of v1 onto v2:
            // v1_proj = cos(angle) * |v1| * norm(v2)

            glm::mat4 thisLocalTransform   = thisTransform.get_model_transform() * this->m_localTransform.get_model_transform();
            glm::mat3 thisNormalTransform  = glm::transpose(glm::inverse(glm::mat3(thisLocalTransform)));
            glm::mat4 otherLocalTransform  = otherTransform.get_model_transform() * other->m_localTransform.get_model_transform();
            glm::mat3 otherNormalTransform = glm::transpose(glm::inverse(glm::mat3(otherLocalTransform)));

            std::array<glm::vec3, 15> axes;
            // each calculated interval must be comparable, so they need to be using
            //   equal sized basis vector (aka normalized)
            // we'll wait until the axes are tested to normalize incase of early exits

            // this' face normals (w=0.0f -> no translation)
            axes[0] = thisNormalTransform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            axes[1] = thisNormalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            axes[2] = thisNormalTransform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            // other's face normals (w=0.0f -> no translation)
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
            // get velocity of this relative to other (thisVelocity - otherVelocity)
            // extend the interval of this by that relative velocity

            for (int a = 0; a < 15; a++)
            {
                // wait as late as possible to normalize each axis
                axes[a] = glm::normalize(axes[a]);
                // CHECK! if colliders are alligned cross products might not work!
                if (isnan(axes[a].x))
                    continue;

                glm::vec2 intervalA =  this->project_self(thisLocalTransform,  axes[a]);
                glm::vec2 intervalB = other->project_self(otherLocalTransform, axes[a]);

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
                    //negate axis
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
            this->build_contact_manifold(thisLocalTransform, -collisionNormal, collisionDepth, thisContactManifold);

            std::vector<glm::vec3> otherContactManifold;
            other->build_contact_manifold(otherLocalTransform, collisionNormal, collisionDepth, otherContactManifold);

            // Solve for collisionPoint depending on size of manifolds found
            if (otherContactManifold.size() == 0 || thisContactManifold.size() == 0)
            {
                PLEEPLOG_ERROR("Contact Manifold(s) could not be built somehow?!");
                return false;
            }
            else if (otherContactManifold.size() == 1)
            {
                // single penetrating vertex on other
                // prefer using single vertex of other first
                collisionPoint = otherContactManifold.front();
                return true;
            }
            else if (thisContactManifold.size() == 1)
            {
                // single penetrating vertex on this
                collisionPoint = thisContactManifold.front();
                return true;
            }
            else
            {
                //PLEEPLOG_WARN("Non trivial Manifolds... expect something to go wrong!");

                // use this' manifold to clip other's manifold
                IColliderComponent::pseudo_clip_polyhedra(thisContactManifold, otherContactManifold, collisionNormal);

                // something went wrong :(
                assert(!otherContactManifold.empty());
                // TODO: for "production" we may want a emergency clause to use simple average if clipping fails

                // now other is completely clipped, use it to find "average" contact point

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

                // scale other points' weight based on its dot product to collisionNormal
                // linearly scaling out to some min % at a distance of m_manifoldDepth
                const float manifoldRange = 1.0f - m_manifoldMinWeight;
                
                collisionPoint = glm::vec3(0.0f);
                for (glm::vec3 v : otherContactManifold)
                {
                    const float vertCoeff = glm::dot(v, collisionNormal);
                    const glm::vec3 relativeVertex = v - manifoldOrigin;
                    // scale dot product coefficient to range [0 : manifoldDepth],
                    // then scale that range to range [0 : 1.0]
                    float vertWeight = (vertCoeff-maxCoeff+m_manifoldDepth) / m_manifoldDepth;
                    // make weight non-linear?
                    //vertWeight *= vertWeight;
                    // then scale that range to range [minWeight : 1.0]
                    collisionPoint += relativeVertex * (vertWeight * manifoldRange + m_manifoldMinWeight);
                }
                collisionPoint /= otherContactManifold.size();
                collisionPoint += manifoldOrigin;

                return true;
            }

            // we should never get here
            return false;
        }

    private:
        // ***** intersection helper methods *****

        // return the coefficents (lengths) of the interval of the box collider's projection along an axis
        glm::vec2 project_self(const glm::mat4& thisTrans, const glm::vec3& axis) const
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
                            this->m_dimensions.x * i,
                            this->m_dimensions.y * j,
                            this->m_dimensions.z * k
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

        // fill dest with all points on plane perpendicular and farthest along axis
        // Manifold must be returned in winding order around the perimeter
        void build_contact_manifold(const glm::mat4& thisTrans, const glm::vec3 axis, const float depth, std::vector<glm::vec3>& dest) const
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
                    this->m_dimensions.x * dimensions.x,
                    this->m_dimensions.y * dimensions.y,
                    this->m_dimensions.z * dimensions.z
                };

                // we have to do this matrix multiply again...
                vertex = thisTrans * glm::vec4(vertex, 1.0f);

                const float coeff = glm::dot(vertex, axis);
                maxCoeff = coeff > maxCoeff ? coeff : maxCoeff;

                // We can greedily cull points GUARENTEED to be outside
                // though depending on order recieved this can miss points
                // but it is a *slight* optimization
                if (coeff >= maxCoeff - m_manifoldDepth)
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
                if (it->first >= maxCoeff - m_manifoldDepth)
                {
                    dest.push_back(it->second);
                }
            }
        }
    };
}

#endif // BOX_COLLIDER_COMPONENT_H