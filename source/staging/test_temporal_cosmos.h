#ifndef TEST_TEMPORAL_COSMOS_H
#define TEST_TEMPORAL_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"

#include "core/cosmos_builder.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_temporal_cosmos(EventBroker* eventBroker, I_NetworkDynamo* networkDynamo)
    {
        // TODO: receive config from file?
        CosmosBuilder::Config cosmosConfig;
        cosmosConfig.components[0] = CosmosBuilder::ComponentType::transform;
        cosmosConfig.components[1] = CosmosBuilder::ComponentType::physics;
        cosmosConfig.components[2] = CosmosBuilder::ComponentType::box_collider;
        cosmosConfig.components[3] = CosmosBuilder::ComponentType::ray_collider;
        cosmosConfig.components[4] = CosmosBuilder::ComponentType::rigid_body;
        cosmosConfig.components[5] = CosmosBuilder::ComponentType::spring_body;
        cosmosConfig.components[6] = CosmosBuilder::ComponentType::script;

        // build cosmos according to config
        CosmosBuilder generator;
        std::shared_ptr<Cosmos> cosmos = generator.generate(cosmosConfig, networkDynamo->get_timeslice_id(), eventBroker, nullptr, nullptr, nullptr, networkDynamo, nullptr);

        UNREFERENCED_PARAMETER(networkDynamo);

        Entity time = cosmos->create_entity();
        PLEEPLOG_DEBUG("(time)            Entity: " + std::to_string(time));
        PLEEPLOG_DEBUG("(time)  host TimesliceId: " + std::to_string(derive_timeslice_id(time)));
        PLEEPLOG_DEBUG("(time)         GenesisId: " + std::to_string(derive_genesis_id(time)));
        PLEEPLOG_DEBUG("(time)   CausalChainLink: " + std::to_string(derive_causal_chain_link(time)));
        PLEEPLOG_DEBUG("(time)    instance count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(time)));

        Entity space = cosmos->create_entity();
        PLEEPLOG_DEBUG("(space)            Entity: " + std::to_string(space));
        PLEEPLOG_DEBUG("(space)  host TimesliceId: " + std::to_string(derive_timeslice_id(space)));
        PLEEPLOG_DEBUG("(space)         GenesisId: " + std::to_string(derive_genesis_id(space)));
        PLEEPLOG_DEBUG("(space)   CausalChainLink: " + std::to_string(derive_causal_chain_link(space)));
        PLEEPLOG_DEBUG("(space)    instance count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(time)));

        return cosmos;
    }
}

#endif // TEST_TEMPORAL_COSMOS_H