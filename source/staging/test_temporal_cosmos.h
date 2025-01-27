#ifndef TEST_TEMPORAL_COSMOS_H
#define TEST_TEMPORAL_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"
#include "staging/cosmos_builder.h"
#include "ecs/ecs_types.h"

#include "staging/hard_config_cosmos.h"

namespace pleep
{
    inline std::shared_ptr<Cosmos> build_test_temporal_cosmos(
        std::shared_ptr<EventBroker> eventBroker, 
        DynamoCluster& dynamoCluster
    )
    {
/* 
        // TODO: receive config from file?
        cosmos_builder::Config cosmosConfig;
        cosmosConfig.insert_component<TransformComponent>();
        cosmosConfig.insert_component<PhysicsComponent>();
        cosmosConfig.insert_component<ColliderComponent>();
        cosmosConfig.insert_component<BehaviorsComponent>();

        // build cosmos according to config
        std::shared_ptr<Cosmos> cosmos = cosmos_builder::generate(cosmosConfig, dynamoCluster, eventBroker);
 */
        std::shared_ptr<Cosmos> cosmos = construct_hard_config_cosmos(eventBroker, dynamoCluster);

        Entity time = cosmos->create_entity();
        UNREFERENCED_PARAMETER(time);
        PLEEPLOG_INFO("(time)            Entity: " + std::to_string(time));
        PLEEPLOG_INFO("(time)  host TimesliceId: " + std::to_string(derive_timeslice_id(time)));
        PLEEPLOG_INFO("(time)         GenesisId: " + std::to_string(derive_genesis_id(time)));
        PLEEPLOG_INFO("(time)   CausalChainLink: " + std::to_string(derive_causal_chain_link(time)));
        PLEEPLOG_INFO("(time)    instance count: " + std::to_string(cosmos->get_hosted_entity_count(time)));

        Entity space = cosmos->create_entity();
        UNREFERENCED_PARAMETER(space);
        PLEEPLOG_INFO("(space)            Entity: " + std::to_string(space));
        PLEEPLOG_INFO("(space)  host TimesliceId: " + std::to_string(derive_timeslice_id(space)));
        PLEEPLOG_INFO("(space)         GenesisId: " + std::to_string(derive_genesis_id(space)));
        PLEEPLOG_INFO("(space)   CausalChainLink: " + std::to_string(derive_causal_chain_link(space)));
        PLEEPLOG_INFO("(space)    instance count: " + std::to_string(cosmos->get_hosted_entity_count(time)));


         // ***************************************************************************
        ModelCache::ImportReceipt frog_import = ModelCache::import("..\\intercession_design\\adult-maloncremia-oot3dmm3d\\source\\3DS - The Legend of Zelda Majoras Mask 3D - Cremia\\cremia.dae");

        Entity frog = cosmos->create_entity();

        //cosmos->add_component(frog, MetaComponent{ "froog" });
        cosmos->add_component(frog, TransformComponent(glm::vec3(6.0f, 2.0f, -0.5f)));
        //cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.001f, 0.001f, 0.001f);
        
        RenderableComponent frog_renderable;
        for (std::string meshName : frog_import.meshNames)
        {
            frog_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        for (std::string matName : frog_import.materialNames)
        {
            frog_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }

        //std::shared_ptr<Model> frog_model = model_builder::create_cube("resources/container.jpg");
        cosmos->add_component(frog, frog_renderable);
        PhysicsComponent frog_physics;
        frog_physics.mass = 30.0f;
        frog_physics.isAsleep = true;
        //frog_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.2f);
        cosmos->add_component(frog, frog_physics);

        ColliderComponent frog_collider;

        // frog "body"
        //frog_collier.colliders[0].localTransform.scale = glm::vec3(5.0f, 4.0f, 5.0f);
        frog_collider.colliders[0] = Collider(ColliderType::box, CollisionType::rigid);
        frog_collider.colliders[0].influenceOrientation = false;

        // frog "legs"
        frog_collider.colliders[1] = Collider(ColliderType::ray, CollisionType::spring);
        frog_collider.colliders[1].localTransform.orientation = glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        //frog_collider.colliders[1].localTransform.scale = glm::vec3(1.0f, 1.0f, 5.0f);
        frog_collider.colliders[1].inheritOrientation = false;
        frog_collider.colliders[1].useBehaviorsResponse = true;
        frog_collider.colliders[1].influenceOrientation = false;
        frog_collider.colliders[1].stiffness = 10000.0f;
        frog_collider.colliders[1].damping = 500.0f;
        frog_collider.colliders[1].restLength = 0.1f; // therefore ride height of 0.9
        frog_collider.colliders[1].staticFriction = 0.0f;
        frog_collider.colliders[1].dynamicFriction = 0.0f;

        cosmos->add_component(frog, frog_collider);
        
        // behaviors handle legs collider events
        BehaviorsComponent frog_behaviors;
        frog_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::biped_control);
        frog_behaviors.use_fixed_update = true;
        // store behaviors in self
        cosmos->add_component(frog, frog_behaviors);
        // ***************************************************************************


        // ***************************************************************************
        Entity light = cosmos->create_entity();
        cosmos->add_component(light, TransformComponent(glm::vec3(0.0f, 1.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(light).scale = glm::vec3(0.2f);
        // remember this is relative to exposure
        cosmos->add_component(light, LightSourceComponent(glm::vec3(4.0f, 4.0f, 4.0f)));
        
        RenderableComponent light_renderable;
        light_renderable.meshData = ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron);
        ModelCache::create_material("lightbulb_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse,  "resources/blending_transparent_window.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
            //{TextureType::height,   "resources/snow-packed12-Height.png"},
            {TextureType::emissive, "resources/snow-packed12-Specular.png"}
        });
        light_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(light, light_renderable);
        
        BehaviorsComponent light_behaviors;
        light_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::oscillator);
        light_behaviors.use_fixed_update = true;
        cosmos->add_component(light, light_behaviors);
        OscillatorComponent light_oscillator;
        cosmos->add_component(light, light_oscillator);
        // ***************************************************************************


        // ***************************************************************************
        // Scene needs to create an entity with camera component
        Entity mainCamera = cosmos->create_entity();
        cosmos->add_component(mainCamera, TransformComponent(glm::vec3(5.0f, 2.5f, 6.0f)));
        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        cosmos->add_component(mainCamera, mainCamera_physics);
        cosmos->get_component<TransformComponent>(mainCamera).orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, -0.2f));
        cosmos->add_component(mainCamera, CameraComponent());

        cosmos->add_component(mainCamera, SpacialInputComponent());
        BehaviorsComponent camera_behaviors;
        camera_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::fly_control);
        camera_behaviors.use_fixed_update = true;
        cosmos->add_component(mainCamera, camera_behaviors);

        // then it needs to be assigned somewhere in render pipeline (view camera, shadow camera, etc)
        // assuming there is only ever 1 main camera we can notify over event broker
        // dynamos don't have access to cosmos, so they can't lookup entity
        // synchro can maintain camera and pass its data each frame
        
        EventMessage cameraEvent(events::rendering::SET_MAIN_CAMERA);
        events::rendering::SET_MAIN_CAMERA_params cameraParams {
            mainCamera
        };
        cameraEvent << cameraParams;
        eventBroker->send_event(cameraEvent);
        // ***************************************************************************


        return cosmos;
    }
}

#endif // TEST_TEMPORAL_COSMOS_H