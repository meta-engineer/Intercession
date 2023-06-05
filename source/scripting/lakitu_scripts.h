#ifndef LAKITU_SCRIPTS_H
#define LAKITU_SCRIPTS_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"
#include "scripting/i_script_drivetrain.h"
#include "scripting/script_component.h"
#include "physics/transform_component.h"
#include "inputting/spacial_input_component.h"
#include "rendering/camera_component.h"
#include "scripting/biped_component.h"
#include "core/cosmos.h"

namespace pleep
{
    // scripts for controling non-physical camera to follow an entity as well
    // as accepting input to change view of the target entity
    class LakituScripts : public I_ScriptDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, ScriptComponent& script, Entity entity = NULL_ENTITY, std::shared_ptr<Cosmos> owner = nullptr) override
        {
            UNREFERENCED_PARAMETER(deltaTime);
            UNREFERENCED_PARAMETER(script);


            // fetch my Transform, SpacialInput, and Camera components
            try
            {
                TransformComponent& transform = owner->get_component<TransformComponent>(entity);
                //PhysicsComponent& physics = owner->get_component<PhysicsComponent>(entity);
                //SpacialInputComponent& input = owner->get_component<SpacialInputComponent>(entity);
                CameraComponent& camera = owner->get_component<CameraComponent>(entity);

                
                // generate direction vector from euler angles
                glm::vec3 direction = transform.get_heading();
/* 
                // units/time * time (seconds)
                //const float disp   = 4.0f * (float)deltaTime;
                const float rot    = 0.10f * (float)deltaTime;
                const float aspect = 1.2f;
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
 */

                glm::vec3 targetOrigin(0.0f);
                glm::quat targetOrientation = glm::quat(glm::vec3(0.0f));
                try
                {
                    if (camera.target != NULL_ENTITY)
                    {
                        TransformComponent& targetTransform = owner->get_component<TransformComponent>(camera.target);
                        targetOrigin = targetTransform.origin;

                        BipedComponent& targetBiped = owner->get_component<BipedComponent>(camera.target);
                        targetOrientation = targetBiped.aimOrientation;
                    }
                }
                catch(const std::exception& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    // if no target just use static coordinates
                }

                // match orientation to target
                transform.orientation = targetOrientation;

                // recalc direction after rotation
                direction = transform.get_heading();
                // set origin to point towards target (or 0 if no target)
                transform.origin = targetOrigin - (direction * camera.range);
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform/SpacialInput/Camera) for entity " + std::to_string(entity) + ". This script cannot run without them. Disabling this drivetrain's on_fixed_update script");
                script.use_fixed_update = false;
                return;
            }
        }        
    };
}

#endif // LAKITU_SCRIPTS_H