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
#include "core/cosmos.h"

namespace pleep
{
    // behaviors for controling non-physical camera to follow an entity as well
    // as accepting input to change view of the target entity
    class OsrsCameraBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner) override
        {
            UNREFERENCED_PARAMETER(deltaTime);

            std::shared_ptr<Cosmos> cosmos = owner.expired() ? nullptr : owner.lock();
            if (!cosmos) return;    // how was owner null, but BehaviorsPacket has a component REFERENCE?

            // fetch my Transform, SpacialInput, and Camera components
            try
            {
                TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
                //PhysicsComponent& physics = cosmos->get_component<PhysicsComponent>(entity);
                SpacialInputComponent& input = cosmos->get_component<SpacialInputComponent>(entity);
                CameraComponent& camera = cosmos->get_component<CameraComponent>(entity);
                
                glm::vec3 direction = transform.get_heading();

                // turn when middle mouse button on
                // TODO: make this proportional to FOV%
                //     E.G. if mouse covers 50% of screen, which is 90*, turn should be 45*
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
                    camera.range = std::max(std::min(camera.range, 20.0f), 5.0f);
                }

                // keep centered on target
                glm::vec3 targetOrigin(0.0f);
                try
                {
                    if (camera.target != NULL_ENTITY)
                    {
                        TransformComponent& targetTransform = cosmos->get_component<TransformComponent>(camera.target);
                        targetOrigin = targetTransform.origin;
                    }
                }
                catch(const std::exception& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    // if no target just use static coordinates
                }

                // recalc direction after rotation
                direction = transform.get_heading();

                // set origin to point towards target (or 0 if no target)
                // TODO: make this parametric so camera movement isn't identical to target
                transform.origin = targetOrigin - (direction * camera.range);
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
    };
}

#endif // OSRS_CAMERA_BEHAVIORS_H