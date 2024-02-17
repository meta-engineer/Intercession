#ifndef OSRS_CAMERA_BEHAVIORS_H
#define OSRS_CAMERA_BEHAVIORS_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "behaviors/behaviors_component.h"
#include "physics/transform_component.h"
#include "inputting/spacial_input_component.h"
#include "rendering/camera_component.h"
#include "behaviors/biped_component.h"
#include "physics/ray_collider_component.h"
#include "core/cosmos.h"

namespace pleep
{
    // behaviors for controling non-physical camera to follow an entity as well
    // as accepting input to change view of the target entity
    class OsrsCameraBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner, std::shared_ptr<EventBroker> sharedBroker) override
        {
            UNREFERENCED_PARAMETER(deltaTime);
            UNREFERENCED_PARAMETER(sharedBroker);

            std::shared_ptr<Cosmos> cosmos = owner.lock();
            // how was owner null, but BehaviorsPacket has a component REFERENCE?
            assert(!owner.expired());

            // fetch my Transform, SpacialInput, and Camera components
            try
            {
                TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
                //PhysicsComponent& physics = cosmos->get_component<PhysicsComponent>(entity);
                SpacialInputComponent& input = cosmos->get_component<SpacialInputComponent>(entity);
                CameraComponent& camera = cosmos->get_component<CameraComponent>(entity);
                RayColliderComponent& ray = cosmos->get_component<RayColliderComponent>(entity);
                
                glm::vec3 direction = transform.get_heading();

                // store raycast values when available
                if (input.actions.test(SpacialActions::raycastX) &&
                    input.actions.test(SpacialActions::raycastY))
                {
                    camera.cursor.x = static_cast<float>(input.actionVals.at(SpacialActions::raycastX));
                    camera.cursor.y = static_cast<float>(input.actionVals.at(SpacialActions::raycastY));
                }

                // set raycast when right mouse button
                if (input.actions.test(SpacialActions::action1))
                    //&& input.actionVals.at(SpacialActions::action1))   // only proc on edge (press)
                {
                    //PLEEPLOG_DEBUG("mouse at " + std::to_string(camera.cursor.x) + ", " + std::to_string(camera.cursor.y));

                    // ray will already inherit cameras heading, so now just orient it as if in view space


                    // mouse positive is right and down
                    // world-space positive is starboard(right), mast(up), and stern(backward)
                    // default direction is {0,0,1} so
                    // view-space positive is port(left), mast(up), and bow(forward)

                    // adjust cursor based on aspect ratio
                    glm::vec4 viewplaneCursor = {camera.cursor.x, camera.cursor.y, 0.0f, 1.0f};
                    glm::vec3 cursorVec = glm::inverse(get_projection(camera)) * -viewplaneCursor;

                    glm::vec3 axis = glm::normalize(glm::cross(cursorVec, glm::vec3(0,0,1)));
                    float dot = glm::dot(glm::normalize(cursorVec), glm::vec3(0,0,1));
                    float angle = glm::acos(dot);
                    ray.localTransform.orientation = glm::angleAxis(-angle, axis);
                    
                    ray.isActive = true;
                }
                else
                {
                    ray.isActive = false;
                }
                

                // turn when middle mouse button on
                // TODO: make this proportional to FOV%
                //     E.G. if mouse covers 50% of screen, which is 90*, turn should be 45*
                // TODO: camera resets after jumping timelines so may have to save it between transfers?
                if (input.actions.test(SpacialActions::action2))
                {
                    // units/time * time (seconds)
                    //const float disp   = 4.0f * (float)deltaTime;
                    const float rot    = 0.20f * (float)deltaTime;
                    const float aspect = 1.2f;  // increase in yaw compared to pitch
                    //const float gimbalLimit = 0.1f;  // rads
                    glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
                    glm::vec3 tangent = glm::normalize(glm::cross(direction, gimbalUp));

                    if (input.actions.test(SpacialActions::rotatePitch))
                    {
                        transform.orientation = glm::angleAxis(rot * (float)input.actionVals.at(SpacialActions::rotatePitch), -tangent) * transform.orientation;
                    }

                    if (input.actions.test(SpacialActions::rotateYaw))
                    {
                        transform.orientation = glm::angleAxis(rot * aspect * (float)input.actionVals.at(SpacialActions::rotateYaw), -gimbalUp) * transform.orientation;
                    }
                    
                }
                // can use scroll anytime
                if (input.actions.test(SpacialActions::rotateRoll))
                {
                    camera.range -= (float)input.actionVals.at(SpacialActions::rotateRoll);
                    // std::clamp
                    camera.range = std::max(std::min(camera.range, 50.0f), 2.0f);
                }

                
                // recalc direction after rotation
                direction = transform.get_heading();


                // keep centered on target
                // TODO: add static target position
                glm::vec3 targetOrigin(0.0f);
                try
                {
                    if (camera.target != NULL_ENTITY)
                    {
                        TransformComponent& targetTransform = cosmos->get_component<TransformComponent>(camera.target);
                        targetOrigin = targetTransform.origin;
                        
                        // set origin to point towards target
                        // TODO: make this parametric so camera movement isn't identical to target
                        transform.origin = targetOrigin - (direction * camera.range);
                    }
                }
                catch(const std::exception& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    // if no target just use static coordinates
                    camera.target = NULL_ENTITY;
                }


            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform/SpacialInput/Camera) for entity " + std::to_string(entity) + ". This behaviors cannot run without them. Disabling this drivetrain's on_fixed_update behaviors");
                behaviors.use_fixed_update = false;
                return;
            }
        }
        
        void on_collision(ColliderPacket callerData, ColliderPacket collidedData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint, std::shared_ptr<EventBroker> sharedBroker) override
        {
            // "mouse raycast" ray collider will call here as caller
            // which means collision metadata is relative to the hit object

            // TODO: store collision meta-data in collider components (if depth is closest)
            // also accept parameter for collision point relative velocity from relay?
            // dynamo should clear collision data at frame start.

            // fetch biped component on collider
            try
            {
                //PLEEPLOG_DEBUG("Mouse collision with entity " + std::to_string(collidedData.collidee) + " at " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z) + " with normal " + std::to_string(collisionNormal.x) + ", " + std::to_string(collisionNormal.y) + ", " + std::to_string(collisionNormal.z));

                std::shared_ptr<Cosmos> cosmos = callerData.owner.lock();
                if (callerData.owner.expired()) return;

                //RayColliderComponent& ray = cosmos->get_component<RayColliderComponent>(callerData.collidee);
                CameraComponent& camera = cosmos->get_component<CameraComponent>(callerData.collidee);

                // need to make collision available as a position for the target entity
                // we need to write it into the entities' SpacialInputComponent
                // which is then carried upstream to the server next frame
                // BUT input dynamo will overritw input components so it needs to be written to the raw input buffer
                // BUT BUT the raw input buffer is cleared at the end of the frame...

                if (cosmos->has_component<SpacialInputComponent>(camera.target))
                {
                    // 3D analog input event? received by input dynamo?
                    EventMessage odmEvent(events::window::VIRTUAL_ODM_GEAR_INPUT);
                    events::window::VIRTUAL_ODM_GEAR_INPUT_params odmInfo{ 
                        collisionPoint.x,
                        collisionPoint.y,
                        collisionPoint.z
                    };
                    odmEvent << odmInfo;

                    // broker acces passed from collision physics relay
                    sharedBroker->send_event(odmEvent);
                }

                UNREFERENCED_PARAMETER(collisionPoint);
                UNREFERENCED_PARAMETER(collisionDepth);
                UNREFERENCED_PARAMETER(collisionNormal);
                UNREFERENCED_PARAMETER(collidedData);
            }
            catch (const std::runtime_error& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch a Ray Component for entity " + std::to_string(callerData.collidee) + " calling behaviors. This behaviors cannot operate on this entity without it. Disabling caller's collider behaviors response.");
                callerData.collider->useBehaviorsResponse = false;
            }
        }       
    };
}

#endif // OSRS_CAMERA_BEHAVIORS_H