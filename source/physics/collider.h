#ifndef COLLIDER_H
#define COLLIDER_H

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
    // Collider composed transform will apply to a "unit" shape of these kinds:
    enum class ColliderType
    {
        none,
        //AABB,
        box, // unit cube, use localTransform.scale to set side lengths
        sphere, // unit sphere (diameter 1)
        ray, // unit vector (0,0,1), use localTransform to rotate and scale ray
        //capsule,
        //cylinder,
        //mesh,
        count
    };

    // Any CollisionType other than noop requires that entity to have a PhysicsComponent
    enum class CollisionType
    {
        noop,       // (only behaviors trigger)
        rigid,      // Static & dynamic resolution
        spring,     // Apply dynamic force away from static COM
        force,      // Apply static force in dynamic direction
        //soft,       // Deform collider
        count
    };

    class Collider
    {
    public:
        Collider() {};
        Collider(ColliderType shape, CollisionType response)
            : colliderType(shape)
            , collisionType(response)
            , isActive(true)
        {};
        ~Collider() = default;

        
        // ***** Universal collider attributes *****

        // Inform physics dynamo how to determine intersections with this collider
        ColliderType colliderType = ColliderType::box;
        // Inform physics dynamo for how to respond to a detected collision
        CollisionType collisionType = CollisionType::rigid;
        // Flag for external components to temporarily disable without overriding collisionType
        bool isActive = false;
        // if true call on_collision for this entity's behaviors drivetrain when collision occurs
        bool useBehaviorsResponse = false;

        // nested transform component "offset" (in local space) from entity's origin
        // Transform scale makes geometrically defined collider shapes
        // NOTE: centre of mass may or may not correlate
        TransformComponent localTransform;
        // Entity can have unlocked orientation, but this collider maintains alignment
        bool inheritOrientation = true;
        // You may also want the collision response to not influence orientation
        bool influenceOrientation = true;
        
        // Track parametric value for CLOSEST collision to avoid multiple collisions
        // Synchro resets this value upon submitting at the start of each frame
        // TODO: is this order dependant? can multiple collisions still occur?
        float minParametricValue = 1.0f;
        void reset()
        {
            minParametricValue = 1.0f;
        }

        // Material properies
        // proportion of energy *lost* through friction
        float staticFriction    = 0.45f;
        float dynamicFriction   = 0.45f;
        // proportion of energy *retained* normal response [0,1]
        float restitution       = 0.70f;
        float stiffness         = 100.0f;
        float damping           = 10.0f;
        // proportion of length of the collider FROM the origin force is applied relative to
        float restLength        = 0.1f;

        
        // "Getter" for inertia tensor, accepts inherited scale
        // Does not include mass or density
        glm::mat3 get_inertia_tensor(glm::vec3 scale = glm::vec3(1.0f)) const
        {
            scale = scale * localTransform.scale;

            switch(this->colliderType)
            {
            case ColliderType::box:
            {
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
            break;
            case ColliderType::ray:
            {
                glm::mat3 I(0.0f);

                // assumes rod is along z axis
                // coefficient of 3 is "real", lower (more resistant) may be needed for stability
                I[0][0] = (scale.z*scale.z) / 3.0f;
                I[1][1] = (scale.z*scale.z) / 3.0f;
                // we need to pretend the ray is a cylinder with some non-zero radius
                I[2][2] = (0.0001f);
                return I;
            }
            break;
            default:
            {
                // we'll use a unit sphere as default
                const float radius = glm::min(glm::min(scale.x, scale.y), scale.z);
                return glm::mat3(radius*radius * (2.0f/5.0f));
            }
            break;
            }
        }
        
        // Combine parent transform with localTransform according to collider's options
        // return product of transform matrices
        glm::mat4 compose_transform(TransformComponent parentTransform) const
        {
            // modify transform copy with options
            if (!inheritOrientation)
            {
                // this is NOT a reference on purpose!
                parentTransform.orientation = glm::quat(glm::vec3(0.0f));
            }
            // allow non-uniform scaling
            return parentTransform.get_model_transform() * localTransform.get_model_transform();
        }
    };
}

#endif // COLLIDER_H