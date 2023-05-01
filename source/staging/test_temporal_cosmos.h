#ifndef TEST_TEMPORAL_COSMOS_H
#define TEST_TEMPORAL_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"

#include "events/event_broker.h"
#include "staging/dynamo_cluster.h"
#include "staging/cosmos_builder.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_temporal_cosmos(
        EventBroker* eventBroker, 
        DynamoCluster& dynamoCluster)
    {
        // TODO: receive config from file?
        cosmos_builder::Config cosmosConfig;
        cosmosConfig.insert_component<TransformComponent>();
        cosmosConfig.insert_component<PhysicsComponent>();
        cosmosConfig.insert_component<BoxColliderComponent>();
        cosmosConfig.insert_component<RayColliderComponent>();
        cosmosConfig.insert_component<RigidBodyComponent>();
        cosmosConfig.insert_component<SpringBodyComponent>();
        cosmosConfig.insert_component<ScriptComponent>();

        // build cosmos according to config
        std::shared_ptr<Cosmos> cosmos = cosmos_builder::generate(cosmosConfig, dynamoCluster, eventBroker);

        Entity time = cosmos->create_entity();
        UNREFERENCED_PARAMETER(time);
        PLEEPLOG_DEBUG("(time)            Entity: " + std::to_string(time));
        PLEEPLOG_DEBUG("(time)  host TimesliceId: " + std::to_string(derive_timeslice_id(time)));
        PLEEPLOG_DEBUG("(time)         GenesisId: " + std::to_string(derive_genesis_id(time)));
        PLEEPLOG_DEBUG("(time)   CausalChainLink: " + std::to_string(derive_causal_chain_link(time)));
        PLEEPLOG_DEBUG("(time)    instance count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(time)));

        Entity space = cosmos->create_entity();
        UNREFERENCED_PARAMETER(space);
        PLEEPLOG_DEBUG("(space)            Entity: " + std::to_string(space));
        PLEEPLOG_DEBUG("(space)  host TimesliceId: " + std::to_string(derive_timeslice_id(space)));
        PLEEPLOG_DEBUG("(space)         GenesisId: " + std::to_string(derive_genesis_id(space)));
        PLEEPLOG_DEBUG("(space)   CausalChainLink: " + std::to_string(derive_causal_chain_link(space)));
        PLEEPLOG_DEBUG("(space)    instance count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(time)));

        return cosmos;
    }
}

#endif // TEST_TEMPORAL_COSMOS_H