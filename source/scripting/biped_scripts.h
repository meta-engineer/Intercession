#ifndef BIPED_SCRIPTS_H
#define BIPED_SCRIPTS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "scripting/i_script_drivetrain.h"
#include "core/cosmos.h"
#include "physics/physics_component.h"
#include "scripting/biped_component.h"
#include "inputting/spacial_input_component.h"

namespace pleep
{
    class BipedScripts : public I_ScriptDrivetrain
    {
    public:
        BipedScripts()
        {
            this->enable_collision_scripts = true;
            this->enable_fixed_update = true;
        }
        
        void on_fixed_update(double deltaTime, Entity entity = NULL_ENTITY, Cosmos* owner = nullptr) override
        {
            // should Biped "control" be done here?
            // (we fetch input component, physics/transform, and a standalon biped component and operate on them)
            // or should "controlling" have a dedicated synchro-dynamo-relay pipeline?

            // network, input, and render all need to share common resources
            // and physics need to interact between entities
            // however, controlling is independant of other entities so it could be done
            // in a standalone script

            // if script components could store multiple I_ScriptDrivetrains then we could run as many as needed
            // and if commonly used scripts could be shared through smart pointers, it could be more memory conservative and use one object for all (like relays)

            // fetch Physics, Biped, and SpacialInput
            try
            {
                PhysicsComponent& physics = owner->get_component<PhysicsComponent>(entity);
                BipedComponent& biped = owner->get_component<BipedComponent>(entity);
                SpacialInputComponent& input = owner->get_component<SpacialInputComponent>(entity);

                // TODO: copy basic_biped_control_relay
                UNREFERENCED_PARAMETER(deltaTime);
                UNREFERENCED_PARAMETER(physics);
                UNREFERENCED_PARAMETER(biped);
                UNREFERENCED_PARAMETER(input);
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_ERROR(err.what());
                PLEEPLOG_ERROR("Could not fetch components (Transform, Biped, and/or SpacialInput) for entity " + std::to_string(entity) + ". This script cannot run without them. Disabling this drivetrain's on_fixed_update script");
                this->enable_fixed_update = false;
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
                // TODO: store collision meta-data in biped component
                BipedComponent& biped = callerData.owner->get_component<BipedComponent>(callerData.collidee);
                biped.isGrounded = true;
                biped.groundNormal = collisionNormal;

                UNREFERENCED_PARAMETER(collisionPoint);
                UNREFERENCED_PARAMETER(collisionNormal);
                UNREFERENCED_PARAMETER(collisionDepth);
                UNREFERENCED_PARAMETER(collidedData);
            }
            catch (const std::runtime_error& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch a Biped Component for entity " + std::to_string(callerData.collidee) + " calling script. Disabling this drivetrain's collision scripts");
                this->enable_collision_scripts = false;
            }
        }
    };
}

#endif // BIPED_SCRIPTS_H