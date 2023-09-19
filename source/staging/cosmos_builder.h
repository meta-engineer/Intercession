#ifndef COSMOS_BUILDER_H
#define COSMOS_BUILDER_H

//#include "intercession_pch.h"
#include <memory>
#include <typeinfo>
#include <set>

#include "core/cosmos.h"

// include Dynamos
#include "core/dynamo_cluster.h"

// include Synchros
#include "inputting/spacial_input_synchro.h"
#include "rendering/render_synchro.h"
#include "rendering/lighting_synchro.h"
#include "physics/physics_synchro.h"
#include "physics/box_collider_synchro.h"
#include "physics/ray_collider_synchro.h"
#include "networking/network_synchro.h"
#include "behaviors/behaviors_synchro.h"

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
#include "networking/spacetime_component.h"

// include Behaviors
#include "behaviors/behaviors_component.h"
#include "behaviors/oscillator_component.h"
#include "behaviors/projectile_component.h"
#include "behaviors/biped_component.h"

namespace pleep
{
    // Utility for constructing Cosmoses in a standardized way
    // Clients/Servers will use shared CosmosBuilder configs to synchronize between each other
    // Cosmoses should remain independant of any particular synchros/components
    // But the builder will have to explicitly have all usable types included and mapped
    namespace cosmos_builder
    {
        // Describe all the initial conditions (registrations) for a Cosmos
        // must abide by ECS capacities
        class Config
        {
        public:
            Config() = default;
            Config(std::vector<std::string> componentNames, std::vector<std::string> synchroNames)
                : components(componentNames)
                , synchros(synchroNames)
            {}

            // Tries to insert c into config components
            // Returns false if components capacity is reached
            template<typename T_Component>
            bool insert_component()
            {
                std::string c = typeid(T_Component).name();
                if (components.size() >= MAX_COMPONENT_TYPES) return false;

                components.push_back(c);
                return false;
            }
            // Inserts s into config synchros
            template<typename T_Synchro>
            bool insert_synchro()
            {
                std::string s = typeid(T_Synchro).name();
                synchros.push_back(s);
                return false;
            }
            // components are in order so this search will be slow
            template<typename T_Component>
            bool contains_component() const
            {
                std::string c = typeid(T_Component).name();
                for (std::string see : components)
                {
                    if (c == see) return true;
                }
                return false;
            }
            template<typename T_Synchro>
            bool contains_synchro() const
            {
                std::string s = typeid(T_Synchro).name();
                for (std::string ess : components)
                {
                    if (s == ess) return true;
                }
                return false;
            }

            void clear()
            {
                components.clear();
                synchros.clear();
            }

            friend bool operator==(const Config& lhs, const Config& rhs)
            {
                if (lhs.components.size() != rhs.components.size()) return false;
                if (lhs.synchros.size() != rhs.synchros.size()) return false;

                // components must be ordered
                for (size_t c = 0; c < lhs.components.size(); c++)
                {
                    if (lhs.components[c] != rhs.components[c]) return false;
                }
                // synchros dont HAVE to be ordered, but for ease, lets assume they are
                for (size_t s = 0; s < lhs.synchros.size(); s++)
                {
                    if (lhs.synchros[s] != rhs.synchros[s]) return false;
                }
                return true;
            }

            friend std::shared_ptr<Cosmos> generate(cosmos_builder::Config& config, DynamoCluster& callerDynamos, std::shared_ptr<EventBroker> callerBroker);

        protected:
            // Registered Components (in order of registry)
            std::vector<std::string> components;
            // Registered Synchros
            std::vector<std::string> synchros;
        };

        #define COSMOS_CONFIG_DELIMITER "|"
                
        // Dynamic memory makes Config non-POD, so we must override Message serialization
        template<typename T_Msg>
        Message<T_Msg>& operator<<(Message<T_Msg>& msg, const Config& data)
        {
            // concat string lists into single string and stream normally
            std::string componentConcat;
            for (std::string c : data.components)
            {
                componentConcat.append(c + COSMOS_CONFIG_DELIMITER);
            }
            msg << componentConcat;
            
            std::string synchroConcat;
            for (std::string s : data.synchros)
            {
                synchroConcat.append(s + COSMOS_CONFIG_DELIMITER);
            }
            msg << synchroConcat;

            return msg;
        }
        template<typename T_Msg>
        Message<T_Msg>& operator>>(Message<T_Msg>& msg, Config& data)
        {
            // Config is overridden
            data.clear();

            // Read out concatted string
            std::string synchroConcat;
            msg >> synchroConcat;
            // split on known delimiter and fill config
            std::string synchroName;
            while (!synchroConcat.empty())
            {
                synchroName = synchroConcat.substr(
                    synchroConcat.begin(),
                    synchroConcat.find_first_of(COSMOS_CONFIG_DELIMITER)
                );
                synchroConcat.erase(
                    synchroConcat.begin(),
                    synchroConcat.find_first_of(COSMOS_CONFIG_DELIMITER)
                );
                if (synchroName) data.synchros.push_back(synchroName);
            }

            // Do same for components
            std::string componentConcat;
            msg >> componentConcat;
            std::string componentName;
            while (!componentConcat.empty())
            {
                componentName = componentConcat.substr(
                    componentConcat.begin(),
                    componentConcat.find_first_of(COSMOS_CONFIG_DELIMITER)
                );
                componentConcat.erase(
                    componentConcat.begin(),
                    componentConcat.find_first_of(COSMOS_CONFIG_DELIMITER)
                );
                if (componentName) data.synchros.push_back(componentName);
            }

            return msg;
        }


        // Called by CosmosContext to setup their held Cosmos
        // use config parameters to setup
        // return shared pointer? Context may need to regenerate config
        std::shared_ptr<Cosmos> generate(
            cosmos_builder::Config& config,
            DynamoCluster& callerDynamos,
            std::shared_ptr<EventBroker> callerBroker = nullptr
        );

        // Can we retroactively generate a config from an existing cosmos?
        // or should context just save its config?
        // If a cosmos was changed after we built it, we should be able to capture it
        cosmos_builder::Config scan(std::shared_ptr<Cosmos> cosmos);
    }
}

#endif // COSMOS_BUILDER_H