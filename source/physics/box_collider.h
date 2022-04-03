#ifndef BOX_COLLIDER_H
#define BOX_COLLIDER_H

//#include "intercession_pch.h"
#include <string>
#include <array>

#include "physics/i_collider.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class BoxCollider : public ICollider
    {
    public:
        BoxCollider()
            : ICollider(ColliderType::box)
            , m_dimensions(glm::vec3(0.5f))
        {
        }
        // dimensions is actually optional with default glm::vec3(0.5f) (aka. all sidelengths 1.0)
        BoxCollider(glm::vec3 dimensions)
            : BoxCollider()
        {
            m_dimensions = dimensions;
        }

        // Double dispatch other
        bool intersects(
            const ICollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override
        {
            if (other->intersects(this, otherTransform, thisTransform, collisionNormal, collisionDepth, collisionPoint))
            {
                // any integral return values should be inverted due to double dispatch
                collisionNormal *= -1.0f;
                return true;
            }
            return false;
        }
        
        bool intersects(
            const BoxCollider* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) const override
        {
            //PLEEPLOG_DEBUG("Testing collision between types: box and box");

            // TODO: dynamic switch between algorithms

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

            glm::mat4 thisLocalTransform   = thisTransform.get_model_transform();
            glm::mat3 thisNormalTransform  = glm::transpose(glm::inverse(glm::mat3(thisLocalTransform)));
            glm::mat4 otherLocalTransform  = otherTransform.get_model_transform();
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
            this->build_contact_manifold(thisLocalTransform, -collisionNormal, thisContactManifold);
            PLEEPLOG_DEBUG("Found Contact Manifold A of size " + std::to_string(thisContactManifold.size()));
            PLEEPLOG_DEBUG("Example of A: " + std::to_string(thisContactManifold[0].x) + ", " + std::to_string(thisContactManifold[0].y) + ", " + std::to_string(thisContactManifold[0].z));

            std::vector<glm::vec3> otherContactManifold;
            other->build_contact_manifold(otherLocalTransform, collisionNormal, otherContactManifold);
            PLEEPLOG_DEBUG("Found Contact Manifold B of size " + std::to_string(otherContactManifold.size()));
            PLEEPLOG_DEBUG("Example of B: " + std::to_string(otherContactManifold[0].x) + ", " + std::to_string(otherContactManifold[0].y) + ", " + std::to_string(otherContactManifold[0].z));

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
            }
            else if (thisContactManifold.size() == 1)
            {
                // single penetrating vertex on this
                collisionPoint = thisContactManifold.front();
            }
            else
            {
                //ICollider::pseudo_clip_polyhedra(thisContactManifold, otherContactManifold, collisionNormal);
                PLEEPLOG_WARN("Non trivial Manifolds... expect something to go wrong!");

                // now other is completely clipped
                collisionPoint = glm::vec3(0.0f);
                for (glm::vec3 v : otherContactManifold)
                    collisionPoint += v;
                collisionPoint /= otherContactManifold.size();
            }
            
            return true;
        }

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
        // should we also include points +/- manifoldEpsilon?
        void build_contact_manifold(const glm::mat4& thisTrans, const glm::vec3 axis, std::vector<glm::vec3>& dest) const
        {
            assert(dest.empty());

            // do we want a range to account for floating point errors?
            //const float manifoldEpsilon = 0.001f;
            //std::vector<std::pair<float, glm::vec3>> allVertices;

            float maxCoeff = -INFINITY;
            
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

                        // we have to do this matrix multiply again...
                        vertex = thisTrans * glm::vec4(vertex, 1.0f);

                        const float coeff = glm::dot(vertex, axis);
                        if (coeff == maxCoeff)
                        {
                            dest.push_back(vertex);
                        }
                        else if (coeff > maxCoeff)
                        {
                            maxCoeff = coeff;
                            dest.clear();
                            dest.push_back(vertex);
                        }

                        /*
                        allVertices.push_back({
                            std::pair<float, glm::vec3>(glm::dot(vertex, axis), vertex)
                        });
                        */
                    }
                }
            }

            // sort low to high
            /*
            std::sort(allVertices.begin(), allVertices.end(), 
                [](std::pair<float, glm::vec3>& a, std::pair<float, glm::vec3>& b) 
                {
                    return a.first < b.first;
                }
            );
            */
            /*
            for (std::vector<std::pair<float, glm::vec3>>::reverse_iterator it = allVertices.rbegin(); it != allVertices.rend(); ++it)
            {
                if (it->first >= allVertices.back().first - manifoldEpsilon)
                {
                    dest.push_back(it->second);
                }
            }
            */
        }

        // Does not include mass or density
        virtual glm::mat3 getInertiaTensor() const override
        {
            glm::mat3 I(0.0f);
            // x=width, y=height, z=depth
            I[0][0] = (m_dimensions.y*m_dimensions.y + m_dimensions.z*m_dimensions.z)/12.0f;
            I[1][1] = (m_dimensions.x*m_dimensions.x + m_dimensions.z*m_dimensions.z)/12.0f;
            I[2][2] = (m_dimensions.x*m_dimensions.x + m_dimensions.y*m_dimensions.y)/12.0f;
            return I;
        }

        // distance each face is from origin (in local space) like a radius
        // opposite faces will be uniform distance from entity origin
        glm::vec3 m_dimensions;
    };
}

#endif // BOX_COLLIDER_H