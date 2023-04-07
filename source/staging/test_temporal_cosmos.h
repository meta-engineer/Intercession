#ifndef TEST_TEMPORAL_COSMOS_H
#define TEST_TEMPORAL_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"

#include "core/cosmos_builder.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_temporal_cosmos(EventBroker* eventBroker, PhysicsDynamo* physicsDynamo, I_NetworkDynamo* networkDynamo)
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
        std::shared_ptr<Cosmos> cosmos = generator.generate(cosmosConfig, networkDynamo->get_timeslice_id(), eventBroker, nullptr, nullptr, physicsDynamo, networkDynamo, nullptr);

        UNREFERENCED_PARAMETER(physicsDynamo);
        UNREFERENCED_PARAMETER(networkDynamo);

        Entity time = cosmos->create_temporal_entity();
        std::pair<TemporalEntity,CausalChainLink> timeTempId = cosmos->get_temporal_identifier(time);
        PLEEPLOG_DEBUG("(time)  local    Entity: " + std::to_string(time));
        PLEEPLOG_DEBUG("(time)  temporal Entity: " + std::to_string(timeTempId.first));
        PLEEPLOG_DEBUG("(time)  causalchainlink: " + std::to_string(timeTempId.second));
        PLEEPLOG_DEBUG("(time)  temporal  count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(timeTempId.first)));

        Entity space = cosmos->create_local_entity();
        std::pair<TemporalEntity,CausalChainLink> spaceTempId = cosmos->get_temporal_identifier(space);
        PLEEPLOG_DEBUG("(space) local    Entity: " + std::to_string(space));
        PLEEPLOG_DEBUG("(space) temporal Entity: " + std::to_string(spaceTempId.first));
        PLEEPLOG_DEBUG("(space) causalchainlink: " + std::to_string(spaceTempId.second));
        PLEEPLOG_DEBUG("(space) temporal  count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(spaceTempId.first)));

        return cosmos;
    }
}

#endif // TEST_TEMPORAL_COSMOS_H