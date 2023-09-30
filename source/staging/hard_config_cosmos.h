#ifndef TEST_HARD_CONFIG_H
#define TEST_HARD_CONFIG_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"
#include "staging/cosmos_builder.h"

namespace pleep
{
    inline std::shared_ptr<Cosmos> construct_hard_config_cosmos(
        std::shared_ptr<EventBroker> eventBroker,
        DynamoCluster& dynamoCluster
    )
    {
        // TODO: receive config from server
        // TODO: Should config synchros just imply necessary components? Some are 1-to-1 like physics component, some are many-to-one like transform, some are unique like oscillator
/* 
        cosmos_builder::Config cosmosConfig;
        cosmosConfig.insert_component<TransformComponent>();
        cosmosConfig.insert_component<SpacialInputComponent>();
        cosmosConfig.insert_component<RenderableComponent>();
        cosmosConfig.insert_component<CameraComponent>();
        cosmosConfig.insert_component<LightSourceComponent>();
        cosmosConfig.insert_component<PhysicsComponent>();
        cosmosConfig.insert_component<BoxColliderComponent>();
        cosmosConfig.insert_component<RayColliderComponent>();
        cosmosConfig.insert_component<RigidBodyComponent>();
        cosmosConfig.insert_component<SpringBodyComponent>();
        cosmosConfig.insert_component<BehaviorsComponent>();
        cosmosConfig.insert_component<OscillatorComponent>();

        cosmosConfig.insert_synchro<SpacialInputSynchro>();
        cosmosConfig.insert_synchro<LightingSynchro>();
        cosmosConfig.insert_synchro<RenderSynchro>();
        cosmosConfig.insert_synchro<PhysicsSynchro>();
        cosmosConfig.insert_synchro<BoxColliderSynchro>();
        cosmosConfig.insert_synchro<RayColliderSynchro>();
        cosmosConfig.insert_synchro<NetworkSynchro>();
        cosmosConfig.insert_synchro<BehaviorsSynchro>();
        // build cosmos according to config
        //std::shared_ptr<Cosmos> cosmos = cosmos_builder::generate(cosmosConfig, dynamoCluster, eventBroker);
 */

        TimesliceId localTimesliceId = dynamoCluster.networker ? dynamoCluster.networker->get_timeslice_id() : NULL_TIMESLICEID;

        std::shared_ptr<Cosmos> newCosmos = std::make_shared<Cosmos>(eventBroker, localTimesliceId);

        newCosmos->register_component<TransformComponent>();
        newCosmos->register_component<SpacialInputComponent>(ComponentCategory::upstream);
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
        newCosmos->register_component<ProjectileComponent>();
        newCosmos->register_component<BipedComponent>();
        newCosmos->register_component<SpacetimeComponent>();

        newCosmos->register_synchro<SpacialInputSynchro>()->attach_dynamo(dynamoCluster.inputter);
        newCosmos->register_synchro<LightingSynchro>()->attach_dynamo(dynamoCluster.renderer);
        newCosmos->register_synchro<RenderSynchro>()->attach_dynamo(dynamoCluster.renderer);
        newCosmos->register_synchro<PhysicsSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<BoxColliderSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<RayColliderSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<NetworkSynchro>()->attach_dynamo(dynamoCluster.networker);
        newCosmos->register_synchro<BehaviorsSynchro>()->attach_dynamo(dynamoCluster.behaver);
        newCosmos->register_synchro<SpacetimeSynchro>()->attach_dynamo(dynamoCluster.networker);

        return newCosmos;
    }
}

#endif // TEST_HARD_CONFIG_H