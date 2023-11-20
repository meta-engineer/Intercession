#ifndef I_COLLIDER_COMPONENT_H
#define I_COLLIDER_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "logging/pleep_log.h"
#include "physics/transform_component.h"
#include "ecs/ecs_types.h"
#include "events/message.h"

namespace pleep
{
    // subclasses need to "register" themselves here
    enum class ColliderType
    {
        none,
        //AABB,
        box,
        //sphere,
        ray,
        //mesh
    };
    
    // Forward declare all class types to dispatch to
    //struct AABBColliderComponent;
    struct BoxColliderComponent;
    //struct SphereColliderComponent;
    struct RayColliderComponent;
    //struct MeshColliderComponent;

    // define collision behaviours, each type must dispatch to a different behaviour component
    enum CollisionResponseType
    {
        noop,       // (or behaviors trigger only)
        rigid,      // find & call entity's Rigid Body component
        spring,     // find & call entity's Spring Body component
        force,      // find & call entity's Force Generator component
        //soft,       // find & call entity's Soft Body component
    };

    struct I_ColliderComponent
    {
    protected:
        I_ColliderComponent() = default;
    public:
        virtual ~I_ColliderComponent() = default;

        // ***** Universal collider attributes *****
        // !!! Make sure to update serializers at page end if members are updated !!!
        static const uint32_t dataSize = 0
            + sizeof(CollisionResponseType) 
            + sizeof(bool) 
            + sizeof(bool)
            + sizeof(TransformComponent)
            + sizeof(bool);

        // Suggestion to Dynamo for how to respond to a detected collision
        // (which body component to fetch for on the same entity)
        // This may be best implemented with a table/map between response type pairs and functions
        CollisionResponseType responseType = CollisionResponseType::rigid;

        // if true, in addition to collision responseType, also call on_collision for this entity's behaviors drivetrain
        bool useBehaviorsResponse = false;

        // Flag for external components to temporarily disable
        bool isActive = true;

        // nested transform component "offset" (in local space) from entity's origin
        // Transform scale makes geometrically defined collider shapes
        // NOTE: centre of mass may or may not correlate
        TransformComponent localTransform;
        // Entity can have unlocked orientation, but this collider maintains alignment
        //  (you may also want the collision response to not influence orientation)
        bool inheritOrientation = true;
        
        // "Getter" for inertia tensor, accepts inherited scale
        // Does not include mass or density
        virtual glm::mat3 get_inertia_tensor(glm::vec3 scale = glm::vec3(1.0f)) const
        {
            scale = scale * localTransform.scale;
            // we'll use a unit sphere as default
            const float radius = glm::min(glm::min(scale.x, scale.y), scale.z);
            return glm::mat3(radius*radius * (2.0f/5.0f));
        }
        
        // "Read-only" type
        // no default implementation, subclasses must have a registered type!
        // Virtual functions ARE SLOWER, but we actually need dispatch from interface pointer
        // can be replaced with unprotected variable for optimization
        virtual const ColliderType get_type() const = 0;

        // ***** Intersection methods *****
        // TODO: virtual method dispatching may be inefficient
        // It may be optimal to have a custom function dispatch table in the dynamo...

        // Double-Dispatch derived colliders
        // each dispatch step should return the collision metadata as such:
        // collisionNormal will always be in the direction of other -> this
        // collisionPoint will always be on surface of other
        // collisionDepth -> surface of this = collisionPoint - (collisionDepth * collisionNormal)
        // For now specify non-continuous (static) time intersection detection
        virtual bool static_intersect(
            I_ColliderComponent* other, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint) = 0;

        virtual bool static_intersect(
            BoxColliderComponent* otherBox, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint)
        {
            PLEEPLOG_WARN("No implementation for collision between this type (?) and BoxColliderComponent");
            UNREFERENCED_PARAMETER(otherBox);
            UNREFERENCED_PARAMETER(thisTransform);
            UNREFERENCED_PARAMETER(otherTransform);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
            return false;
        }
        
        virtual bool static_intersect(
            RayColliderComponent* otherRay, 
            const TransformComponent& thisTransform,
            const TransformComponent& otherTransform,
            glm::vec3& collisionNormal,
            float& collisionDepth,
            glm::vec3& collisionPoint)
        {
            PLEEPLOG_WARN("No implementation for collision between this type (?) and RayColliderComponent");
            UNREFERENCED_PARAMETER(otherRay);
            UNREFERENCED_PARAMETER(thisTransform);
            UNREFERENCED_PARAMETER(otherTransform);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
            return false;
        }

        // ***** collider helper utils *****

        // Combine parent transform with localTransform according to collider's options
        // return product of transform matrices
        glm::mat4 compose_transform(TransformComponent parentTransform) const
        {
            // modify transform copy with options
            if (!inheritOrientation)
            {
                parentTransform.orientation = glm::quat(glm::vec3(0.0f));
            }
            // allow non-uniform scaling
            return parentTransform.get_model_transform() * localTransform.get_model_transform();
        }

        // TODO: move this out of this class, it is not really related and bloating this file
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
    };
        
    // Virtual dispatch makes I_ColliderComponent non-POD, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const I_ColliderComponent& data)
    {
        // make sure stream operators are updated if members are updated
        static_assert(I_ColliderComponent::dataSize == 47, "I_ColliderComponent Message serializer found unexpected data size");
        
        uint32_t i = static_cast<uint32_t>(msg.size());
        // resize all at once
        msg.body.resize(msg.body.size() + I_ColliderComponent::dataSize);

        std::memcpy(msg.body.data() + i, &(data.responseType), sizeof(CollisionResponseType));
        i += sizeof(CollisionResponseType);
        
        std::memcpy(msg.body.data() + i, &(data.isActive), sizeof(bool));
        i += sizeof(bool);
        
        std::memcpy(msg.body.data() + i, &(data.useBehaviorsResponse), sizeof(bool));
        i += sizeof(bool);
        
        std::memcpy(msg.body.data() + i, &(data.localTransform), sizeof(TransformComponent));
        i += sizeof(TransformComponent);
        
        std::memcpy(msg.body.data() + i, &(data.inheritOrientation), sizeof(bool));
        i += sizeof(bool);
        
        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, I_ColliderComponent& data)
    {
        // stream out when no data is available;
        assert(msg.size() >= I_ColliderComponent::dataSize);
        
        // track index at the start of the data on "top" of the stack
        uint32_t i = static_cast<uint32_t>(msg.size()) - I_ColliderComponent::dataSize;
        
        std::memcpy(&(data.responseType), msg.body.data() + i, sizeof(CollisionResponseType));
        i += sizeof(CollisionResponseType);
        
        std::memcpy(&(data.isActive), msg.body.data() + i, sizeof(bool));
        i += sizeof(bool);
        
        std::memcpy(&(data.useBehaviorsResponse), msg.body.data() + i, sizeof(bool));
        i += sizeof(bool);
        
        std::memcpy(&(data.localTransform), msg.body.data() + i, sizeof(TransformComponent));
        i += sizeof(TransformComponent);
        
        std::memcpy(&(data.inheritOrientation), msg.body.data() + i, sizeof(bool));
        i += sizeof(bool);

        // shrink, removing end of stack (constant time)
        msg.body.resize(msg.size() - I_ColliderComponent::dataSize);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
        
        return msg;
    }
    
    // Testing
    inline bool operator==(const I_ColliderComponent& lhs, const I_ColliderComponent& rhs)
    {
        return lhs.responseType == rhs.responseType
            && lhs.isActive == rhs.isActive
            //&& lhs.useBehaviorsResponse == rhs.useBehaviorsResponse
            && lhs.localTransform == rhs.localTransform
            && lhs.inheritOrientation == rhs.inheritOrientation;
    }
}

#endif // I_COLLIDER_COMPONENT_H