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
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner) override
        {
            std::shared_ptr<Cosmos> cosmos = owner.lock();
            // how was owner null, but BehaviorsPacket has a component REFERENCE?
            assert(!owner.expired());

            // fetch Projectile
            try
            {
                ProjectileComponent& proj = cosmos->get_component<ProjectileComponent>(entity);

                proj.currLifetime += deltaTime;

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