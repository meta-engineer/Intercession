#ifndef TEST_TEMPORAL_COSMOS_H
#define TEST_TEMPORAL_COSMOS_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"

// TODO: This is temporary for building hard-coded entities
#include "physics/physics_synchro.h"
#include "physics/box_collider_synchro.h"
#include "physics/ray_collider_synchro.h"
#include "networking/network_synchro.h"

#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/box_collider_component.h"
#include "physics/ray_collider_component.h"
#include "physics/rigid_body_component.h"

#include "scripting/script_component.h"
#include "scripting/biped_scripts.h"

namespace pleep
{
    void build_temporal_cosmos(Cosmos* cosmos, EventBroker* eventBroker, PhysicsDynamo* physicsDynamo, I_NetworkDynamo* networkDynamo)
    {
        // register components
        // components will have to be the superset of server & client for signatures to match
        cosmos->register_component<TransformComponent>();
        cosmos->register_component<PhysicsComponent>();
        cosmos->register_component<BoxColliderComponent>();
        cosmos->register_component<RayColliderComponent>();
        cosmos->register_component<RigidBodyComponent>();
        cosmos->register_component<SpringBodyComponent>();
        cosmos->register_component<ScriptComponent>();

        UNREFERENCED_PARAMETER(eventBroker);
        UNREFERENCED_PARAMETER(physicsDynamo);
        UNREFERENCED_PARAMETER(networkDynamo);

        
        Entity time = cosmos->create_temporal_entity();
        std::pair<TemporalEntity,CausalChainLink> timeTempId = cosmos->get_temporal_identifier(time);
        PLEEPLOG_DEBUG("time local    Entity: " + std::to_string(time));
        PLEEPLOG_DEBUG("time temporal Entity: " + std::to_string(timeTempId.first));
        PLEEPLOG_DEBUG("time causalchainlink: " + std::to_string(timeTempId.second));
        PLEEPLOG_DEBUG("time temporal  count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(timeTempId.first)));

        Entity space = cosmos->create_local_entity();
        std::pair<TemporalEntity,CausalChainLink> spaceTempId = cosmos->get_temporal_identifier(space);
        PLEEPLOG_DEBUG("space local    Entity: " + std::to_string(space));
        PLEEPLOG_DEBUG("space temporal Entity: " + std::to_string(spaceTempId.first));
        PLEEPLOG_DEBUG("space causalchainlink: " + std::to_string(spaceTempId.second));
        PLEEPLOG_DEBUG("space temporal  count: " + std::to_string(cosmos->get_hosted_temporal_entity_count(spaceTempId.first)));
    }
}

#endif // TEST_TEMPORAL_COSMOS_H