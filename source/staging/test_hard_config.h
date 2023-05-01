#ifndef TEST_HARD_CONFIG_H
#define TEST_HARD_CONFIG_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "staging/dynamo_cluster.h"
#include "staging/cosmos_builder.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_test_hard_config(
        EventBroker* eventBroker,
        DynamoCluster& dynamoCluster
    )
    {
        TimesliceId localTimesliceId = dynamoCluster.networker ? dynamoCluster.networker->get_timeslice_id() : NULL_TIMESLICEID;

        std::shared_ptr<Cosmos> newCosmos = std::make_shared<Cosmos>(eventBroker, localTimesliceId);

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
        newCosmos->register_component<ScriptComponent>();
        newCosmos->register_component<OscillatorComponent>();

        newCosmos->register_synchro<SpacialInputSynchro>()->attach_dynamo(dynamoCluster.inputter);
        newCosmos->register_synchro<LightingSynchro>()->attach_dynamo(dynamoCluster.renderer);
        newCosmos->register_synchro<RenderSynchro>()->attach_dynamo(dynamoCluster.renderer);
        newCosmos->register_synchro<PhysicsSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<BoxColliderSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<RayColliderSynchro>()->attach_dynamo(dynamoCluster.physicser);
        newCosmos->register_synchro<NetworkSynchro>()->attach_dynamo(dynamoCluster.networker);
        newCosmos->register_synchro<ScriptSynchro>()->attach_dynamo(dynamoCluster.scripter);

        return newCosmos;
    }
}

#endif // TEST_HARD_CONFIG_H