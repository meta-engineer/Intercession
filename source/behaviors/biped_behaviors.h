#ifndef BIPED_BEHAVIORS_H
#define BIPED_BEHAVIORS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "behaviors/behaviors_component.h"
#include "core/cosmos.h"
#include "physics/physics_component.h"
#include "physics/collider_component.h"
#include "behaviors/biped_component.h"
#include "inputting/spacial_input_component.h"
#include "staging/test_projectile.h"

namespace pleep
{
    class BipedBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner, std::shared_ptr<EventBroker> sharedBroker) override
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

            std::shared_ptr<Cosmos> cosmos = owner.lock();
            // how was owner null, but BehaviorsPacket has a component REFERENCE?
            assert(!owner.expired());
            
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
                ColliderComponent& collider = cosmos->get_component<ColliderComponent>(entity);
                BipedComponent& biped = cosmos->get_component<BipedComponent>(entity);
                SpacialInputComponent& input = cosmos->get_component<SpacialInputComponent>(entity);

                const glm::vec3 heading = transform.get_heading();

                // generate aim "heading" from aimOrientation and support vector
                // movement axes while airborn may depend on camera gimbal axis?
                const glm::mat3 aimRotation = glm::toMat4(biped.aimOrientation);
                const glm::vec3 aimHeading = glm::normalize(aimRotation * glm::vec3(0.0f, 0.0f, 1.0f));
                const glm::vec3 aimTangent = glm::normalize(glm::cross(aimHeading, biped.supportAxis));
                const glm::vec3 aimProjection = glm::normalize(glm::cross(biped.supportAxis, aimTangent));
                // derive velocity perpendicular to support axis
                const glm::vec3 planarVelocity = physics.velocity - (glm::dot(physics.velocity, biped.supportAxis) * biped.supportAxis);


                // apply angular acceleration from current orientation towards aimOrientation
                
                float springStiffness = 100.0f;
                float springDamping = 10.0f;
                const float headingDot = glm::dot(heading, aimHeading);
                // if dot is very large we want to weaken springforce
                // if dot is very negative we want to strengthen springforce
                springStiffness *= (1.0f - headingDot) * 2.0f;

                // how to get quat difference from orientation to aimOrientation??
                glm::quat quatdiff = biped.aimOrientation * glm::inverse(transform.orientation);
                glm::vec3 diffAxis = glm::vec3(quatdiff.x, quatdiff.y, quatdiff.z);
                if (glm::length2(diffAxis) != 0.0f)
                {
                    physics.angularAcceleration += glm::normalize(diffAxis) * quatdiff.w * springStiffness;
                }
                // & dampen
                //physics.angularVelocity *= 0.5f;
                physics.angularAcceleration -= physics.angularVelocity * springDamping;

                // don't turn if just sliding slowly (and avoid NaN)
                if (glm::length2(planarVelocity) > 0.1f)
                {
                    // convert planar velocity to quat
                    float planarAngle = glm::acos(glm::dot(glm::normalize(planarVelocity), glm::vec3(0,0,1)));
                    if (planarVelocity.x > 0) planarAngle *= -1.0f;
                    // save valid planar velocity as aim direction
                    biped.aimOrientation = glm::angleAxis(planarAngle, biped.supportAxis);
                }


                // Move towards mouse target:
                // if targetCoord is valid and cooldown < 0 then set cooldown
                // while cooldown > 0, accelerate towards target
                // decrement cooldown
                // if cooldown < 0 set targetCoord invalid
                // if no input, default target to current position
                glm::vec3 targetCoord = transform.origin;

                if (input.actions.test(SpacialActions::targetX))
                {
                    targetCoord = {
                        static_cast<float>(input.actionVals.at(SpacialActions::targetX)),
                        static_cast<float>(input.actionVals.at(SpacialActions::targetY)),
                        static_cast<float>(input.actionVals.at(SpacialActions::targetZ))
                    };
                }

                glm::vec3 targetVector = targetCoord - transform.origin;
                if (glm::length2(targetVector) != 0.0f)
                {
                    glm::vec3 targetCross = glm::cross(biped.supportAxis, targetVector);
                    targetVector = glm::cross(targetCross, biped.supportAxis);
                    targetVector = glm::normalize(targetVector);
                }

                // if grounded accelerate from current velocity towards target
                if (biped.isGrounded)
                {
                    // if aim is > 90 degrees away from direction, then slow movement
                    if (glm::dot(targetVector, aimHeading) < 0.5f)
                    {
                        targetVector *= 0.1f;
                    }

                    glm::vec3 targetGroundVelocity = targetVector * biped.groundTargetSpeed;
                    const glm::vec3 deltaGroundVelocity = targetGroundVelocity - planarVelocity;

                    physics.acceleration += deltaGroundVelocity * biped.groundAcceleration;
                }
                // if not grounded just accelerate
                // also prevent accelerating beyond air max
                else
                {
                    glm::vec3 airAcceleration = targetVector * biped.airAcceleration;
                    
                    /// TODO: Air movement should only be for changing direction, should not be able to accelerate beyond airMaxSpeed

                    physics.acceleration += airAcceleration;
                }


                // TODO: make a proper state machine to manage these conditions and component changes

                // assume second collider is "legs" ray
                Collider& legs = collider.colliders[1];

                // I Have No Legs And I Must Jump
                if (legs.colliderType != ColliderType::ray || 
                    legs.collisionType != CollisionType::spring)
                {
                    // cannot jump
                }
                // while jump is off CD
                else if (biped.jumpCooldownRemaining <= 0)
                {
                    // Move legs restLength to top
                    legs.damping = 400.0f;
                    
                    // try jumping
                    if (biped.isGrounded &&
                        input.actions.test(SpacialActions::moveVertical) &&
                        input.actionVals.at(SpacialActions::moveVertical) > 0)
                    {
                        biped.jumpCooldownRemaining = biped.jumpCooldownTime;

                        legs.restLength = 0.0f;
                        legs.damping = 0.0f;
                    }
                    // try crouching
                    else if (input.actions.test(SpacialActions::moveVertical) &&
                        input.actionVals.at(SpacialActions::moveVertical) < 0)
                    {
                        legs.restLength = 0.6f;
                    }
                    // otherwise standing
                    else
                    {
                        legs.restLength = 0.3f;
                    }
                }
                // jump is on CD (during jump)
                else
                {
                    biped.jumpCooldownRemaining -= deltaTime;
                }


                // clear gounded state until next collision
                biped.isGrounded = false;

/* 
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
 */

                // Back mouse button    
                if (input.actions.test(SpacialActions::action3)
                    && input.actionVals.at(SpacialActions::action3))
                {
                    EventMessage jumpMessage(events::network::JUMP_REQUEST);
                    events::network::JUMP_params jumpInfo{
                        1, entity
                    };
                    jumpMessage << jumpInfo;
                    PLEEPLOG_DEBUG("Jump " + std::to_string(entity) + " Backwards!");

                    // prevent double jump on arrival
                    input.clear();

                    sharedBroker->send_event(jumpMessage);
                }

                // forward mouse button
                if (input.actions.test(SpacialActions::action4)
                    && input.actionVals.at(SpacialActions::action4))
                {
                    EventMessage jumpMessage(events::network::JUMP_REQUEST);
                    events::network::JUMP_params jumpInfo{
                        -1, entity
                    };
                    jumpMessage << jumpInfo;
                    PLEEPLOG_DEBUG("Jump " + std::to_string(entity) + " Forwards!");

                    // prevent double jump on arrival
                    input.clear();

                    sharedBroker->send_event(jumpMessage);
                }
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform, Physics, Collider, Biped, and/or SpacialInput) for entity " + std::to_string(entity) + ". This behaviors cannot operate on this entity without them. Disabling caller's on_fixed_update behaviors.");
                behaviors.use_fixed_update = false;
            }
        }

        void on_collision(ColliderPacket callerData, ColliderPacket collidedData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint, std::shared_ptr<EventBroker> sharedBroker) override
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
                UNREFERENCED_PARAMETER(sharedBroker);
            }
            catch (const std::runtime_error& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch a Biped Component for entity " + std::to_string(callerData.collidee) + " calling behaviors. This behaviors cannot operate on this entity without it. Disabling caller's collider behaviors response.");
                callerData.collider.useBehaviorsResponse = false;
            }
        }
    };
}

#endif // BIPED_BEHAVIORS_H