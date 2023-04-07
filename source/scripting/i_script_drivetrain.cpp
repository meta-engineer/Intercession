#include "scripting/i_script_drivetrain.h"

#include "logging/pleep_log.h"
#include "scripting/script_component.h"

namespace pleep
{
    // Provide noop defaults so subclasses don't need to implement every callback
    // ScriptComponents will disable them by default, but can enable if their subclass has it

    void I_ScriptDrivetrain::on_fixed_update(double deltaTime, ScriptComponent& script, Entity entity, std::shared_ptr<Cosmos> owner)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        UNREFERENCED_PARAMETER(script);
        UNREFERENCED_PARAMETER(entity);
        UNREFERENCED_PARAMETER(owner);
        PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
        script.use_fixed_update = false;
    }
    
    void I_ScriptDrivetrain::on_frame_update(double deltaTime, ScriptComponent& script, Entity entity, std::shared_ptr<Cosmos> owner)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        UNREFERENCED_PARAMETER(script);
        UNREFERENCED_PARAMETER(entity);
        UNREFERENCED_PARAMETER(owner);
        PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
        script.use_frame_update = false;
    }

    void I_ScriptDrivetrain::on_collision(ColliderPacket colliderData, ColliderPacket collideeData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint)
    {
        UNREFERENCED_PARAMETER(colliderData);
        UNREFERENCED_PARAMETER(collideeData);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
        PLEEPLOG_WARN("Drivetrain has no implementation for called script. Disabling...");
        colliderData.collider->scriptTarget = NULL_ENTITY;
    }

    void I_ScriptDrivetrain::on_collision_enter()
    {
        PLEEPLOG_WARN("Drivetrain has no implementation for called script.");
    }
    void I_ScriptDrivetrain::on_collision_exit()
    {
        PLEEPLOG_WARN("Drivetrain has no implementation for called script.");
    }
}