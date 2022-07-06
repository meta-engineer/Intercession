#ifndef BIPED_SCRIPTS_H
#define BIPED_SCRIPTS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "scripting/i_script_drivetrain.h"
#include "controlling/biped_control_component.h"
#include "core/cosmos.h"

namespace pleep
{
    class BipedScripts : public I_ScriptDrivetrain
    {
    public:
        BipedScripts()
        {
            this->enable_collision_scripts = true;
        }

        void on_collision(ColliderPacket callerData, ColliderPacket collidedData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint) override
        {
            // "legs" ray collider will call here as caller
            // which means collision metadata is relative to the "ground"

            // look for biped controller on collider
            try
            {
                BipedControlComponent& biped = callerData.owner->get_component<BipedControlComponent>(callerData.collidee);
                biped.isGrounded = true;
                biped.groundNormal = collisionNormal;
            }
            catch (const std::range_error& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch a Biped Controller on entity " + std::to_string(callerData.collidee) + " calling script. Disabling this drivetrain's collision scripts");
                this->enable_collision_scripts = false;
            }

            UNREFERENCED_PARAMETER(collisionPoint);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collidedData);
        }
    };
}

#endif // BIPED_SCRIPTS_H