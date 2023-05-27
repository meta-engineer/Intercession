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
        EventBroker* eventBroker, 
        DynamoCluster& dynamoCluster
    )
    {
/* 
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
 */
        std::shared_ptr<Cosmos> cosmos = construct_hard_config_cosmos(eventBroker, dynamoCluster);

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


         // ***************************************************************************
        ModelCache::ImportReceipt frog_import = ModelCache::import("..\\intercession_design\\adult-maloncremia-oot3dmm3d\\source\\3DS - The Legend of Zelda Majoras Mask 3D - Cremia\\cremia.dae");

        Entity frog = cosmos->create_entity();

        //cosmos->add_component(frog, MetaComponent{ "froog" });
        cosmos->add_component(frog, TransformComponent(glm::vec3(6.0f, 2.0f, -0.5f)));
        //cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.001f, 0.001f, 0.001f);
        
        RenderableComponent frog_renderable;
        if (!frog_import.supermeshNames.empty())
        {
            frog_renderable.meshData = ModelCache::fetch_supermesh(frog_import.supermeshNames.front());
        }
        if (!frog_import.supermeshMaterialsNames.empty())
        {
            for (std::string matName : frog_import.supermeshMaterialsNames.front())
            {
                frog_renderable.materials.push_back(ModelCache::fetch_material(matName));
            }
        }
        //std::shared_ptr<Model> frog_model = model_builder::create_cube("resources/container.jpg");
        cosmos->add_component(frog, frog_renderable);
        PhysicsComponent frog_physics;
        frog_physics.mass = 30.0f;
        frog_physics.isAsleep = true;
        //frog_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.2f);
        cosmos->add_component(frog, frog_physics);

        // frog "body"
        BoxColliderComponent frog_box;
        //frog_box.localTransform.scale = glm::vec3(5.0f, 4.0f, 5.0f);
        frog_box.responseType = CollisionResponseType::rigid;
        cosmos->add_component(frog, frog_box);
        RigidBodyComponent frog_rigidBody;
        frog_rigidBody.influenceOrientation = false;
        cosmos->add_component(frog, frog_rigidBody);

        // script handle legs collider events (below)
        ScriptComponent frog_scripts;
        frog_scripts.drivetrain = ScriptLibrary::fetch_script(ScriptLibrary::ScriptType::biped_control);
        frog_scripts.use_fixed_update = true;
        // store script in self
        cosmos->add_component(frog, frog_scripts);

        // frog "legs"
        RayColliderComponent frog_ray;
        frog_ray.localTransform.orientation = glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        //frog_ray.localTransform.scale = glm::vec3(1.0f, 1.0f, 5.0f);
        frog_ray.responseType = CollisionResponseType::spring;
        frog_ray.inheritOrientation = false;
        // link to self biped script
        frog_ray.scriptTarget = frog;
        cosmos->add_component(frog, frog_ray);
        SpringBodyComponent frog_springBody;
        frog_springBody.influenceOrientation = false;
        frog_springBody.stiffness = 10000.0f;
        frog_springBody.damping = 500.0f;
        frog_springBody.restLength = 0.1f; // therefore ride height of 0.9
        frog_springBody.staticFriction = 0.0f;
        frog_springBody.dynamicFriction = 0.0f;
        cosmos->add_component(frog, frog_springBody);
        // ***************************************************************************


        // ***************************************************************************
        Entity light = cosmos->create_entity();
        cosmos->add_component(light, TransformComponent(glm::vec3(0.0f, 1.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(light).scale = glm::vec3(0.2f);
        // remember this is relative to exposure
        cosmos->add_component(light, LightSourceComponent(glm::vec3(4.0f, 4.0f, 4.0f)));
        
        RenderableComponent light_renderable;
        light_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::icosahedron);
        ModelCache::create_material("lightbulb_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse,  "resources/blending_transparent_window.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
            //{TextureType::height,   "resources/snow-packed12-Height.png"},
            {TextureType::emissive, "resources/snow-packed12-Specular.png"}
        });
        light_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(light, light_renderable);
        
        ScriptComponent light_scripts;
        light_scripts.drivetrain = ScriptLibrary::fetch_script(ScriptLibrary::ScriptType::oscillator);
        light_scripts.use_fixed_update = true;
        cosmos->add_component(light, light_scripts);
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
        ScriptComponent camera_scripts;
        camera_scripts.drivetrain = ScriptLibrary::fetch_script(ScriptLibrary::ScriptType::fly_control);
        camera_scripts.use_fixed_update = true;
        cosmos->add_component(mainCamera, camera_scripts);

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