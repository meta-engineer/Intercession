#include "cosmos_builder.h"

namespace pleep
{
    std::shared_ptr<Cosmos> cosmos_builder::generate(
        cosmos_builder::Config& config,
        DynamoCluster& callerDynamos,
        EventBroker* callerBroker
    )
    {
        static std::unordered_map<const char*, std::function<void(std::shared_ptr<Cosmos>, DynamoCluster&)>> componentRegisters;
        if (componentRegisters.empty())
        {
            componentRegisters.insert({ 
                typeid(TransformComponent).name(), 
                [](std::shared_ptr<Cosmos> cosmos, DynamoCluster& dynamos)
                {
                    UNREFERENCED_PARAMETER(dynamos);
                    cosmos->register_component<TransformComponent>();
                }
            });
        }

        
        PLEEPLOG_TRACE("Derive TimesliceId from network");
        TimesliceId localTimesliceID = callerDynamos.networker ? callerDynamos.networker->get_timeslice_id() : NULL_TIMESLICEID;

        // passthrough broker to Cosmos (null or not)
        PLEEPLOG_TRACE("Create Cosmos");
        std::shared_ptr<Cosmos> newCosmos = std::make_shared<Cosmos>(callerBroker, localTimesliceID);

        PLEEPLOG_TRACE("Register Components");
        for (std::string c : config.components)
        {
            //componentRegisters.at(c.c_str()).operator()(newCosmos, callerDynamos);
        }
/* 
        newCosmos->register_component<TransformComponent>();
        newCosmos->register_component<SpacialInputComponent>();
        newCosmos->register_component<RenderableComponent>();
        newCosmos->register_component<CameraComponent>();
        newCosmos->register_component<LightSourceComponent>();
        newCosmos->register_component<PhysicsComponent>();
        newCosmos->register_component<BoxColliderComponent>();
        newCosmos->register_component<RayColliderComponent>();
        newCosmos->register_component<RigidBodyComponent>();
        newCosmos->register_component<SpringBodyComponent>();
        newCosmos->register_component<BehaviorsComponent>();
        newCosmos->register_component<OscillatorComponent>();
        //PLEEPLOG_WARN("Found unknown component enum value: " + std::to_string(static_cast<uint16_t>(cType)));
*/

        PLEEPLOG_TRACE("Create Synchros");
        for (std::string s : config.synchros)
        {
            //synchroRegisters.at(s.c_str()).operator()(newCosmos, callerDynamos);
        }
/* 
        newCosmos->register_synchro<SpacialInputSynchro>()->attach_dynamo(callerDynamos.inputter);
        newCosmos->register_synchro<LightingSynchro>()->attach_dynamo(callerDynamos.renderer);
        newCosmos->register_synchro<RenderSynchro>()->attach_dynamo(callerDynamos.renderer);
        newCosmos->register_synchro<PhysicsSynchro>()->attach_dynamo(callerDynamos.physicser);
        newCosmos->register_synchro<BoxColliderSynchro>()->attach_dynamo(callerDynamos.physicser);
        newCosmos->register_synchro<RayColliderSynchro>()->attach_dynamo(callerDynamos.physicser);
        newCosmos->register_synchro<NetworkSynchro>()->attach_dynamo(callerDynamos.networker);
        newCosmos->register_synchro<BehaviorsSynchro>()->attach_dynamo(callerDynamos.behaver);
        //PLEEPLOG_WARN("found unknown synchro enum value: " + std::to_string(static_cast<uint16_t>(sType)));
*/
        return newCosmos;
    }
    
    cosmos_builder::Config cosmos_builder::scan(std::shared_ptr<Cosmos> cosmos) 
    {
        // Return data in Config
        return Config(
            cosmos->stringify_component_registry(),
            cosmos->stringify_synchro_registry()
        );
    }
}