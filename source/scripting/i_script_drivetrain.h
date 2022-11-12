#ifndef I_SCRIPT_DRIVETRAIN_H
#define I_SCRIPT_DRIVETRAIN_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "logging/pleep_log.h"
#include "physics/collider_packet.h"

namespace pleep
{
    // Contains all virtual methods that script users can call
    // I guess script users will just... implicitly know which methods they should call
    // EX: on_fixed_update, on_frame_update, on_collision, etc...
    // Scripts should only store members for globally applicable data
    // any entity specific data should be contained in a standalone component
    class I_ScriptDrivetrain
    {
    protected:
        I_ScriptDrivetrain() = default;
    public:
        virtual ~I_ScriptDrivetrain() = default;
        
        // caller behaviour configuration
        // subclasses should enable when they override related method
        bool enable_fixed_update        = false;
        bool enable_frame_update        = false;
        bool enable_collision_scripts   = false;

        virtual void on_fixed_update(double deltaTime, Entity entity = NULL_ENTITY, Cosmos* owner = nullptr)
        {
            UNREFERENCED_PARAMETER(deltaTime);
            UNREFERENCED_PARAMETER(entity);
            UNREFERENCED_PARAMETER(owner);
            PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
            this->enable_fixed_update = false;
        }
        
        virtual void on_frame_update(double deltaTime, Entity entity = NULL_ENTITY, Cosmos* owner = nullptr)
        {
            UNREFERENCED_PARAMETER(deltaTime);
            UNREFERENCED_PARAMETER(entity);
            UNREFERENCED_PARAMETER(owner);
            PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
            this->enable_frame_update = false;
        }

        // we'll copy over data just incase
        // metadata is relative to collidee
        virtual void on_collision(ColliderPacket colliderData, ColliderPacket collideeData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint)
        {
            UNREFERENCED_PARAMETER(colliderData);
            UNREFERENCED_PARAMETER(collideeData);
            UNREFERENCED_PARAMETER(collisionNormal);
            UNREFERENCED_PARAMETER(collisionDepth);
            UNREFERENCED_PARAMETER(collisionPoint);
            PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
            this->enable_collision_scripts = false;
        }

        // TODO: how do we detect/track the "start" and "end" of a collision? should collision relay dispatch?
        virtual void on_collision_enter()
        {
            PLEEPLOG_WARN("Drivetrain has no implementation for called script.");
        }
        virtual void on_collision_exit()
        {
            PLEEPLOG_WARN("Drivetrain has no implementation for called script.");
        }
    };
}

#endif // I_SCRIPT_DRIVETRAIN_H