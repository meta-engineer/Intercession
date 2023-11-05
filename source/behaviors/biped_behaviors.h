#ifndef BIPED_BEHAVIORS_H
#define BIPED_BEHAVIORS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "behaviors/behaviors_component.h"
#include "core/cosmos.h"
#include "physics/physics_component.h"
#include "behaviors/biped_component.h"
#include "inputting/spacial_input_component.h"
#include "staging/test_projectile.h"

namespace pleep
{
    class BipedBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner) override
        {
            // should Biped "control" be done here?
            // (we fetch input component, physics/transform, and a standalon biped component and operate on them)
            // or should "controlling" have a dedicated synchro-dynamo-relay pipeline?

            // network, input, and render all need to share common resources
            // and physics need to interact between entities
            // however, controlling is independant of other entities so it could be done
            // in a standalone behaviors

            // if behaviors components could store multiple I_BehaviorsDrivetrains then we could run as many as needed
            // and if commonly used behaviors could be shared through smart pointers, it could be more memory conservative and use one object for all (like relays)

            std::shared_ptr<Cosmos> cosmos = owner.expired() ? nullptr : owner.lock();
            if (!cosmos) return;    // how was owner null, but BehaviorsPacket has a component REFERENCE?
            
            // precheck for all upstream components
            if (!cosmos->has_component<SpacialInputComponent>(entity))
            {
                return;
            }

            // fetch Physics, Biped, and SpacialInput
            try
            {
                TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
                PhysicsComponent& physics = cosmos->get_component<PhysicsComponent>(entity);
                BipedComponent& biped = cosmos->get_component<BipedComponent>(entity);
                SpacialInputComponent& input = cosmos->get_component<SpacialInputComponent>(entity);

                // generate aim "heading" from aimOrientation and support vector
                // movement axes while airborn may depend on camera gimbal axis?
                const glm::mat3 aimRotation = glm::toMat4(biped.aimOrientation);
                const glm::vec3 aimHeading = glm::normalize(aimRotation * glm::vec3(0.0f, 0.0f, 1.0f));
                const glm::vec3 aimTangent = glm::normalize(glm::cross(aimHeading, biped.supportAxis));
                const glm::vec3 aimProjection = glm::normalize(glm::cross(biped.supportAxis, aimTangent));
                // derive ground velocity perpendicular to support axis
                const glm::vec3 planarVelocity = physics.velocity - (glm::dot(physics.velocity, biped.supportAxis) * biped.supportAxis);

                const float rot    = 0.10f * (float)deltaTime;
                const float aspect = 1.2f;
                // set new aim vectors
                if (input.actions.test(SpacialActions::rotatePitch))
                {
                    biped.aimOrientation = glm::angleAxis(rot * (float)input.actionVals.at(SpacialActions::rotatePitch), aimTangent) * biped.aimOrientation;
                }
                if (input.actions.test(SpacialActions::rotateYaw))
                {
                    biped.aimOrientation = glm::angleAxis(rot * aspect * (float)input.actionVals.at(SpacialActions::rotateYaw), biped.supportAxis) * biped.aimOrientation;
                }

                glm::vec3 targetInputVector(0.0f);
                // omit up/down from accelerations
                if (input.actions.test(SpacialActions::moveParallel))
                {
                    targetInputVector += aimProjection * static_cast<float>(input.actionVals.at(SpacialActions::moveParallel));
                }
                if (input.actions.test(SpacialActions::moveHorizontal))
                {
                    targetInputVector -= aimTangent * static_cast<float>(input.actionVals.at(SpacialActions::moveHorizontal));
                }
                
                if (targetInputVector != glm::vec3(0.0f))
                    targetInputVector = glm::normalize(targetInputVector);
                glm::vec3 targetGroundVelocity = targetInputVector * biped.groundMaxSpeed;
                
                const glm::vec3 deltaGroundVelocity = targetGroundVelocity - planarVelocity;

                physics.acceleration += deltaGroundVelocity * biped.groundAcceleration;


                // jumping
                if (biped.jumpCooldownRemaining > 0) biped.jumpCooldownRemaining -= deltaTime;
                if (biped.jumpCooldownRemaining <= 0
                    && biped.isGrounded
                    && input.actions.test(SpacialActions::moveVertical)
                    && input.actionVals.at(SpacialActions::moveVertical) > 0)
                {
                    biped.jumpCooldownRemaining = biped.jumpCooldownTime;
                    // f = m*a
                    physics.acceleration += -1.0f * biped.supportAxis * biped.jumpForce / physics.mass;
                }
                
                // clear gounded state until next collision
                biped.isGrounded = false;


                // TEST: pew pew
                if (biped.shootCooldownRemaining > 0) biped.shootCooldownRemaining -= deltaTime;
                if (biped.shootCooldownRemaining <= 0
                    && input.actions.test(SpacialActions::action0)
                    && input.actionVals.at(SpacialActions::action0))
                {
                    biped.shootCooldownRemaining = biped.shootCooldownTime;
                    // make cube with velocity along aimHeading
                    create_test_projectile(cosmos, entity, transform.origin+ aimHeading, aimHeading*biped.shootForce);
                }

            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform, Biped, and/or SpacialInput) for entity " + std::to_string(entity) + ". This behaviors cannot operate on this entity without them. Disabling caller's on_fixed_update behaviors.");
                behaviors.use_fixed_update = false;
            }
        }

        void on_collision(ColliderPacket callerData, ColliderPacket collidedData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint) override
        {
            // "legs" ray collider will call here as caller
            // which means collision metadata is relative to the "ground"

            // TODO: store collision meta-data in collider components (if depth is closest)
            // also accept parameter for collision point relative velocity from relay?
            // dynamo should clear collision data at frame start.

            // fetch biped component on collider
            try
            {
                BipedComponent& biped = callerData.owner.lock()->get_component<BipedComponent>(callerData.collidee);
                biped.isGrounded = true;
                biped.groundNormal = collisionNormal;

                UNREFERENCED_PARAMETER(collisionPoint);
                UNREFERENCED_PARAMETER(collisionDepth);
                UNREFERENCED_PARAMETER(collidedData);
            }
            catch (const std::runtime_error& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch a Biped Component for entity " + std::to_string(callerData.collidee) + " calling behaviors. This behaviors cannot operate on this entity without it. Disabling caller's collider behaviors response.");
                callerData.collider->useBehaviorsResponse = false;
            }
        }
    };
}

#endif // BIPED_BEHAVIORS_H