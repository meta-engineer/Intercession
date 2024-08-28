#ifndef PROJECTILE_BEHAVIORS_H
#define PROJECTILE_BEHAVIORS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "behaviors/behaviors_component.h"
#include "core/cosmos.h"
#include "physics/transform_component.h"
#include "behaviors/projectile_component.h"

namespace pleep
{
    class ProjectileBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner, std::shared_ptr<EventBroker> sharedBroker) override
        {
            UNREFERENCED_PARAMETER(sharedBroker);

            std::shared_ptr<Cosmos> cosmos = owner.lock();
            // how was owner null, but BehaviorsPacket has a component REFERENCE?
            assert(!owner.expired());
            assert(deltaTime >= 0);

            // fetch Projectile
            try
            {
                TransformComponent& trans = cosmos->get_component<TransformComponent>(entity);
                ProjectileComponent& proj = cosmos->get_component<ProjectileComponent>(entity);

                proj.currLifetime += deltaTime;

                double sin = glm::sin(proj.currLifetime * proj.hz);
                // sin has min of -1, and amplitude of 1

                double scaleAmplitude = (proj.scaleMax - proj.scaleMin) / 2.0;
                trans.scale = glm::vec3(static_cast<float>(sin * scaleAmplitude + (proj.scaleMin + 1.0)));

                if (proj.currLifetime > proj.maxLifetime)
                {
                    cosmos->condemn_entity(entity, entity);
                }
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Projectile) for entity " + std::to_string(entity) + ". This behaviors cannot operate on this entity without them. Disabling caller's on_fixed_update behaviors.");
                behaviors.use_fixed_update = false;
            }
            
        }
    };
}

#endif // PROJECTILE_BEHAVIORS_H