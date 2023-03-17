#ifndef OSCILLATOR_SCRIPTS_H
#define OSCILLATOR_SCRIPTS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "scripting/script_component.h"
#include "scripting/i_script_drivetrain.h"
#include "core/cosmos.h"
#include "physics/transform_component.h"
#include "scripting/oscillator_component.h"

namespace pleep
{
    class OscillatorScripts : public I_ScriptDrivetrain
    {
    public:
        void on_fixed_update(double deltaTime, ScriptComponent& script, Entity entity = NULL_ENTITY, Cosmos* owner = nullptr) override
        {
            // fetch Oscilator and Transform
            try
            {
                TransformComponent& transform = owner->get_component<TransformComponent>(entity);
                OscillatorComponent& oscillator = owner->get_component<OscillatorComponent>(entity);

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
                PLEEPLOG_WARN("Could not fetch components (Transform, and/or Oscillator) for entity " + std::to_string(entity) + ". This script cannot operate on this entity without them. Disabling caller's on_fixed_update script.");
                script.use_fixed_update = false;
            }
        }
    };
}

#endif // OSCILLATOR_SCRIPTS_H