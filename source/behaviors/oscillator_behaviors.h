#ifndef OSCILLATOR_BEHAVIORS_H
#define OSCILLATOR_BEHAVIORS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "behaviors/behaviors_component.h"
#include "core/cosmos.h"
#include "physics/transform_component.h"
#include "behaviors/oscillator_component.h"

namespace pleep
{
    class OscillatorBehaviors : public I_BehaviorsDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, BehaviorsComponent& behaviors, Entity entity, std::weak_ptr<Cosmos> owner) override
        {
            std::shared_ptr<Cosmos> cosmos = owner.lock();
            // how was owner null, but BehaviorsPacket has a component REFERENCE?
            assert(!owner.expired());

            // fetch Oscilator and Transform
            try
            {
                TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
                OscillatorComponent& oscillator = cosmos->get_component<OscillatorComponent>(entity);

                // This is not a physical integration, so if this entity has collision it might get funky

                // subtract from transform using current phase
                transform.origin.x -= oscillator.amplitude * glm::sin(oscillator.phase / oscillator.period * 2.0f * glm::pi<float>());
                transform.origin.z -= oscillator.amplitude * glm::cos(oscillator.phase / oscillator.period * 2.0f * glm::pi<float>());

                // increment phase
                oscillator.phase = oscillator.phase >= oscillator.period ? oscillator.phase - oscillator.period : oscillator.phase + static_cast<float>(deltaTime);

                // add to transform using new phase
                transform.origin.x += oscillator.amplitude * glm::sin(oscillator.phase / oscillator.period * 2.0f * glm::pi<float>());
                transform.origin.z += oscillator.amplitude * glm::cos(oscillator.phase / oscillator.period * 2.0f * glm::pi<float>());
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                // ComponentRegistry will log error itself
                //PLEEPLOG_WARN(err.what());
                PLEEPLOG_WARN("Could not fetch components (Transform, and/or Oscillator) for entity " + std::to_string(entity) + ". This behaviors cannot operate on this entity without them. Disabling caller's on_fixed_update behaviors.");
                behaviors.use_fixed_update = false;
            }
        }
    };
}

#endif // OSCILLATOR_BEHAVIORS_H