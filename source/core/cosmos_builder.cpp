#include "cosmos_builder.h"

namespace pleep
{
    std::shared_ptr<Cosmos> CosmosBuilder::generate(
        CosmosBuilder::Config config, 
        const TimesliceId localTimesliceIndex,
        EventBroker* callerBroker, 
        RenderDynamo* callerRenderDynamo, 
        InputDynamo* callerInputDynamo, 
        PhysicsDynamo* callerPhysicsDynamo, 
        I_NetworkDynamo* callerNetworkDynamo, 
        ScriptDynamo* callerScriptDynamo
    )
    {
        // passthrough broker to Cosmos (null or not)
        PLEEPLOG_TRACE("Create Cosmos");
        std::shared_ptr<Cosmos> newCosmos = std::make_shared<Cosmos>(callerBroker, localTimesliceIndex);

        PLEEPLOG_TRACE("Register Components");
        for (ComponentType cType : config.components)
        {
            switch(cType)
            {
                case ComponentType::transform:
                    newCosmos->register_component<TransformComponent>();
                    break;
                case ComponentType::spacial_input:
                    newCosmos->register_component<SpacialInputComponent>();
                    break;
                case ComponentType::renderable:
                    newCosmos->register_component<RenderableComponent>();
                    break;
                case ComponentType::camera:
                    newCosmos->register_component<CameraComponent>();
                    break;
                case ComponentType::light_source:
                    newCosmos->register_component<LightSourceComponent>();
                    break;
                case ComponentType::physics:
                    newCosmos->register_component<PhysicsComponent>();
                    break;
                case ComponentType::box_collider:
                    newCosmos->register_component<BoxColliderComponent>();
                    break;
                case ComponentType::ray_collider:
                    newCosmos->register_component<RayColliderComponent>();
                    break;
                case ComponentType::rigid_body:
                    newCosmos->register_component<RigidBodyComponent>();
                    break;
                case ComponentType::spring_body:
                    newCosmos->register_component<SpringBodyComponent>();
                    break;
                case ComponentType::script:
                    newCosmos->register_component<ScriptComponent>();
                    break;
                case ComponentType::oscillator:
                    newCosmos->register_component<OscillatorComponent>();
                    break;
                default:
                    //PLEEPLOG_WARN("Found unknown component enum value: " + std::to_string(static_cast<uint16_t>(cType)));
                    break;
            }
        }

        // synchros are in an unordered map so it isn't guarenteed that LightingSynchro is invoked before RenderSynchro
        // TODO: ordering of synchros in unordered_map DOES AFFECT run order, with undefined, NON-DETERMINISTIC behaviour
        // TODO: Is there a better way to attach dynamos?
        PLEEPLOG_TRACE("Create Synchros");
        for (SynchroType sType : config.synchros)
        {
            switch(sType)
            {
                case SynchroType::spacial_input:
                    newCosmos->register_synchro<SpacialInputSynchro>()->attach_dynamo(callerInputDynamo);
                    break;
                case SynchroType::lighting:
                    newCosmos->register_synchro<LightingSynchro>()->attach_dynamo(callerRenderDynamo);
                    break;
                case SynchroType::render:
                    newCosmos->register_synchro<RenderSynchro>()->attach_dynamo(callerRenderDynamo);
                    break;
                case SynchroType::physics:
                    newCosmos->register_synchro<PhysicsSynchro>()->attach_dynamo(callerPhysicsDynamo);
                    break;
                case SynchroType::box_collider:
                    newCosmos->register_synchro<BoxColliderSynchro>()->attach_dynamo(callerPhysicsDynamo);
                    break;
                case SynchroType::ray_collider:
                    newCosmos->register_synchro<RayColliderSynchro>()->attach_dynamo(callerPhysicsDynamo);
                    break;
                case SynchroType::network:
                    newCosmos->register_synchro<NetworkSynchro>()->attach_dynamo(callerNetworkDynamo);
                    break;
                case SynchroType::script:
                    newCosmos->register_synchro<ScriptSynchro>()->attach_dynamo(callerScriptDynamo);
                    break;
                default:
                    //PLEEPLOG_WARN("found unknown synchro enum value: " + std::to_string(static_cast<uint16_t>(sType)));
                    break;
            }
        }
        
        return newCosmos;
    }
    
    CosmosBuilder::Config CosmosBuilder::scan(std::shared_ptr<Cosmos> cosmos) 
    {
        return Config{};
    }
}