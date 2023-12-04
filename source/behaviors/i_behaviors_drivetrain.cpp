#include "behaviors/i_behaviors_drivetrain.h"

#include "logging/pleep_log.h"
#include "behaviors/behaviors_component.h"

namespace pleep
{
    // Provide noop defaults so subclasses don't need to implement every callback
    // BehaviorsComponents will disable them by default, but can enable if their subclass has it

    void I_BehaviorsDrivetrain::on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        UNREFERENCED_PARAMETER(behaviors);
        UNREFERENCED_PARAMETER(entity);
        UNREFERENCED_PARAMETER(owner);
        PLEEPLOG_WARN("Drivetrain has no implementation for called behaviors. Disabling...");
        behaviors.use_fixed_update = false;
    }
    
    void I_BehaviorsDrivetrain::on_frame_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        UNREFERENCED_PARAMETER(behaviors);
        UNREFERENCED_PARAMETER(entity);
        UNREFERENCED_PARAMETER(owner);
        PLEEPLOG_WARN("Drivetrain has no implementation for called behaviors. Disabling...");
        behaviors.use_frame_update = false;
    }

    void I_BehaviorsDrivetrain::on_collision(ColliderPacket colliderData, ColliderPacket collideeData, glm::vec3 collisionNormal, float collisionDepth, glm::vec3 collisionPoint, std::shared_ptr<EventBroker> sharedBroker)
    {
        UNREFERENCED_PARAMETER(colliderData);
        UNREFERENCED_PARAMETER(collideeData);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);
        UNREFERENCED_PARAMETER(collisionPoint);
        UNREFERENCED_PARAMETER(sharedBroker);
        PLEEPLOG_WARN("Drivetrain has no implementation for called behaviors. Disabling...");
        colliderData.collider->useBehaviorsResponse = false;
    }

    void I_BehaviorsDrivetrain::on_collision_enter()
    {
        PLEEPLOG_WARN("Drivetrain has no implementation for called behaviors.");
    }
    void I_BehaviorsDrivetrain::on_collision_exit()
    {
        PLEEPLOG_WARN("Drivetrain has no implementation for called behaviors.");
    }
}