#ifndef COSMOS_BUILDER_H
#define COSMOS_BUILDER_H

//#include "intercession_pch.h"
#include <memory>
#include <set>

#include "cosmos.h"

// include Synchros
#include "inputting/spacial_input_synchro.h"
#include "rendering/render_synchro.h"
#include "rendering/lighting_synchro.h"
#include "physics/physics_synchro.h"
#include "physics/box_collider_synchro.h"
#include "physics/ray_collider_synchro.h"
#include "networking/network_synchro.h"
#include "scripting/script_synchro.h"

// include Components
#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/box_collider_component.h"
#include "physics/ray_collider_component.h"
#include "physics/rigid_body_component.h"
#include "inputting/spacial_input_component.h"
#include "rendering/renderable_component.h"
#include "rendering/camera_component.h"
#include "rendering/light_source_component.h"
#include "core/meta_component.h"

// include Scripts
#include "scripting/script_component.h"
#include "scripting/oscillator_component.h"

namespace pleep
{
    // Utility for constructing Cosmoses in a standardized way
    // Clients/Servers will use shared CosmosBuilder configs to synchronize between eachother
    // Cosmoses should remain independant of any particular synchros/components
    // But the builder will have to explicitly have all usable types included and enumed
    class CosmosBuilder
    {
    public:
        // Builder will need access to the Contexts dynamos to provide them to built synchros
        //CosmosBuilder();
        //~CosmosBuilder();

        // enumerate known component types which can be built or scanned
        enum class ComponentType : uint16_t
        {
            none,
            transform,
            spacial_input,
            renderable,
            camera,
            light_source,
            physics,
            box_collider,
            ray_collider,
            rigid_body,
            spring_body,
            script,
            oscillator,
            count,
        };
        // enumerate known synchro types which can be built or scanned
        enum class SynchroType : uint16_t
        {
            none,
            spacial_input,
            lighting,
            render,
            physics,
            box_collider,
            ray_collider,
            network,
            script,
            count,
        };

        // Describe all the initial conditions (registrations) for a Cosmos
        // must abide by ECS capacities
        class Config
        {
        public:
            friend class CosmosBuilder;

            // Tries to insert c into config components
            // Returns false if components capacity is reached
            bool insert(ComponentType c)
            {
                if (components.size() >= MAX_COMPONENT_TYPES) return false;

                components.insert(c);
                return false;
            }
            // Inserts s into config synchros
            bool insert(SynchroType s)
            {
                synchros.insert(s);
                return false;
            }
            bool contains(ComponentType c) const
            {
                return components.find(c) != components.end();
            }
            bool contains(SynchroType s) const
            {
                return synchros.find(s) != synchros.end();
            }

            friend bool operator==(const Config& lhs, const Config& rhs)
            {
                if (lhs.components.size() != rhs.components.size()) return false;
                if (lhs.synchros.size() != rhs.synchros.size()) return false;

                for (ComponentType c : lhs.components)
                {
                    if (!rhs.contains(c)) return false;
                }
                for (SynchroType s : lhs.synchros)
                {
                    if (!rhs.contains(s)) return false;
                }
                return true;
            }

        protected:
            // Registered Components
            std::set<ComponentType> components;
            // Registered Synchros
            std::set<SynchroType> synchros;

            // Should Scripts be a part of the cosmos like synchros?
        };

        // Called by CosmosContext to setup their held Cosmos
        // use config parameters to setup
        // return shared pointer? Context may need to regenerate config
        std::shared_ptr<Cosmos> generate(
            CosmosBuilder::Config config,
            EventBroker* callerBroker = nullptr,
            RenderDynamo* callerRenderDynamo = nullptr,
            InputDynamo* callerInputDynamo = nullptr,
            PhysicsDynamo* callerPhysicsDynamo = nullptr,
            I_NetworkDynamo* callerNetworkDynamo = nullptr,
            ScriptDynamo* callerScriptDynamo = nullptr
        );

        // Can we retroactively generate a config from an existing cosmos?
        // or should context just save its config?
        // If a cosmos was changed after we built it, we should be able to capture it
        CosmosBuilder::Config scan(std::shared_ptr<Cosmos> cosmos);

    private:
        
    };
}

#endif // COSMOS_BUILDER_H