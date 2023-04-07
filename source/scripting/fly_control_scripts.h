#ifndef FLY_CONTROL_SCRIPTS_H
#define FLY_CONTROL_SCRIPTS_H

//#include "intercession_pch.h"
#include <glm/gtx/quaternion.hpp>

#include "logging/pleep_log.h"
#include "scripting/i_script_drivetrain.h"
#include "scripting/script_component.h"
#include "scripting/script_packet.h"
#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "inputting/spacial_input_component.h"

namespace pleep
{
    class FlyControlScripts : public I_ScriptDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, ScriptComponent& script, Entity entity = NULL_ENTITY, std::shared_ptr<Cosmos> owner = nullptr) override
        {
            UNREFERENCED_PARAMETER(deltaTime);
            UNREFERENCED_PARAMETER(script);

            // fetch my Transform and SpacialInput components
            try
            {
                TransformComponent& transform = owner->get_component<TransformComponent>(entity);
                //PhysicsComponent& physics = owner->get_component<PhysicsComponent>(entity);
                SpacialInputComponent& input = owner->get_component<SpacialInputComponent>(entity);

                // generate direction vector from euler angles
                glm::vec3 direction = transform.get_heading();
                // units/time * time (seconds)
                const float disp   = 4.0f * (float)deltaTime;
                const float rot    = 0.15f * (float)deltaTime;
                const float aspect = 1.2f;
                //const float gimbalLimit = 0.1f;  // rads
                glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
                glm::vec3 tangent = glm::normalize(glm::cross(direction, gimbalUp));

                transform.origin += direction * disp * (float)(input.actionVals.at(SpacialActions::moveParallel));
                
                transform.origin += tangent * disp * (float)(input.actionVals.at(SpacialActions::moveHorizontal));
                
                transform.origin += gimbalUp * disp * (float)(input.actionVals.at(SpacialActions::moveVertical));

                if (input.actions.test(SpacialActions::rotatePitch))
                {
                    transform.orientation = glm::angleAxis(rot * (float)input.actionVals.at(SpacialActions::rotatePitch), -tangent) * transform.orientation;
                }

                if (input.actions.test(SpacialActions::rotateYaw))
                {
                    transform.orientation = glm::angleAxis(rot * aspect * (float)input.actionVals.at(SpacialActions::rotateYaw), -gimbalUp) * transform.orientation;
                }
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform and/or SpacialInput) for entity " + std::to_string(entity) + ". This script cannot run without them. Disabling this drivetrain's on_fixed_update script");
                script.use_fixed_update = false;
            }
        }
    };
}

#endif // FLY_CONTROL_SCRIPTS_H