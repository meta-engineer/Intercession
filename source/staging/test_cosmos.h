#ifndef TEST_COSMOS_H
#define TEST_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"
#include "staging/cosmos_builder.h"
#include "logging/pleep_log.h"
#include "rendering/model_cache.h"
#include "behaviors/behaviors_library.h"

#include "staging/hard_config_cosmos.h"

namespace pleep
{
    inline std::shared_ptr<Cosmos> build_test_cosmos(
        std::shared_ptr<EventBroker> eventBroker, 
        DynamoCluster& dynamoCluster
    )
    {
        std::shared_ptr<Cosmos> cosmos = construct_hard_config_cosmos(eventBroker, dynamoCluster);
        
        PLEEPLOG_TRACE("Create Entities");
        // create entity then
        // create component and pass or construct inline
        // if component is explicit (no initalizer list), we can omit template

        // ***************************************************************************
        ModelCache::ImportReceipt frog_import = ModelCache::import("..\\intercession_design\\adult-maloncremia-oot3dmm3d\\source\\3DS - The Legend of Zelda Majoras Mask 3D - Cremia\\cremia.dae");
        //ModelCache::import("./resources/12268_banjofrog_v1_L3.obj");
        //return;
        //ModelManager::debug_receipt(frog_import);
        // ***************************************************************************

        // ***************************************************************************
        Entity frog = cosmos->create_entity();

        //cosmos->add_component(frog, MetaComponent{ "froog" });
        cosmos->add_component(frog, TransformComponent(glm::vec3(7.0f, 2.0f, -0.5f)));
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
        //frog_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.2f);
        cosmos->add_component(frog, frog_physics);

        ColliderComponent frog_collider;

        // frog "body"
        frog_collider.colliders[0] = Collider(ColliderType::box, CollisionType::rigid);
        frog_collider.colliders[0].localTransform.scale = glm::vec3(500.0f, 400.0f, 500.0f);
        frog_collider.colliders[0].influenceOrientation = false;

        // frog "legs"
        frog_collider.colliders[1] = Collider(ColliderType::ray, CollisionType::spring);
        frog_collider.colliders[1].localTransform.orientation = glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        frog_collider.colliders[1].localTransform.scale = glm::vec3(1.0f, 1.0f, 500.0f);
        frog_collider.colliders[1].inheritOrientation = false;
        frog_collider.colliders[1].useBehaviorsResponse = true;
        frog_collider.colliders[1].influenceOrientation = false;
        frog_collider.colliders[1].stiffness = 10000.0f;
        frog_collider.colliders[1].damping = 100.0f;
        frog_collider.colliders[1].restLength = 0.1f; // therefore ride height of 0.9
        frog_collider.colliders[1].staticFriction = 0.0f;
        frog_collider.colliders[1].dynamicFriction = 0.0f;

        cosmos->add_component(frog, frog_collider);
        
        // behaviors handle legs collider events (below)
        cosmos->add_component(frog, SpacialInputComponent{});
        cosmos->add_component(frog, BipedComponent{});
        BehaviorsComponent frog_behaviors;
        frog_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::biped_control);
        frog_behaviors.use_fixed_update = true;
        // store behaviors in self
        cosmos->add_component(frog, frog_behaviors);
        // ***************************************************************************

/*
        Entity vamp = cosmos->create_entity();
        cosmos->add_component(vamp, TransformComponent(glm::vec3(2.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(vamp).scale = glm::vec3(0.01f, 0.01f, 0.01f);
        std::shared_ptr<Model> vampModel = std::make_shared<Model>("resources/vampire/dancing_vampire3.dae");
        cosmos->add_component(vamp, ModelComponent(vampModel));
*/

        // ***************************************************************************
        Entity crate = cosmos->create_entity();
        TransformComponent crateTransform(glm::vec3(2.9f, 0.0f, 0.3f));
        cosmos->add_component(crate, crateTransform);

        RenderableComponent crate_renderable;
        crate_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        ModelCache::create_material("crate_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/container2.png"},
            {TextureType::specular, "resources/container2_specular.png"}
        });
        crate_renderable.materials.push_back(ModelCache::fetch_material("crate_mat"));
        cosmos->add_component(crate, crate_renderable);

        PhysicsComponent crate_physics;
        //crate_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        crate_physics.angularVelocity = glm::vec3(0.0f, 0.7f, 0.2f);
        crate_physics.mass = 100.0f;
        cosmos->add_component(crate, crate_physics);
        cosmos->add_component(crate, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        // ***************************************************************************


        /***************************************************************************
        Entity fakeCrate = compose_entity(11, 20, 1);

        EventMessage lanMsg(events::cosmos::ENTITY_UPDATE);
        cosmos->serialize_entity_components(crate, lanMsg);

        cosmos->destroy_entity(crate);

        cosmos->register_entity(fakeCrate);
        cosmos->deserialize_entity_components(fakeCrate, lanMsg, ComponentCategory::all);
        // ***************************************************************************/


        // ***************************************************************************
        Entity block = cosmos->create_entity();

        cosmos->add_component(block, TransformComponent(glm::vec3(2.0f, -1.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).orientation = glm::normalize(glm::angleAxis(glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).scale = glm::vec3(1.8f, 0.3f, 1.8f);

        RenderableComponent block_renderable;
        block_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        ModelCache::create_material("block_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/bricks2.jpg"},
            {TextureType::specular, "resources/bricks2_disp.jpg"},
            {TextureType::normal, "resources/bricks2_normal.jpg"}
        });
        //cosmos->add_component(block, ModelComponent(model_builder::create_cube("resources/bricks2.jpg", "resources/bricks2_disp.jpg", "resources/bricks2_normal.jpg")));
        block_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(block, block_renderable);

        PhysicsComponent block_physics;
        //block_physics.velocity = glm::vec3(0.6f, 0.0f, -0.6f);
        //block_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.3f);
        block_physics.lockOrigin = true;
        block_physics.lockedOrigin = cosmos->get_component<TransformComponent>(block).origin;
        //block_physics.lockOrientation = true;
        //block_physics.lockedOrientation = cosmos->get_component<TransformComponent>(block).orientation;
        block_physics.mass = 500.0f;
        cosmos->add_component(block, block_physics);
        cosmos->add_component(block, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        // ******************************************************************************

        // ***************************************************************************
        Entity torus = cosmos->create_entity();

        TransformComponent torus_transform;
        torus_transform.origin = glm::vec3(0.3f, 1.0f, 0.0f);
        torus_transform.orientation = glm::angleAxis(glm::radians(30.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        //torus_transform.scale = glm::vec3(1.0f, 1.35f, 0.75f);
        cosmos->add_component(torus, torus_transform);
        ModelCache::ImportReceipt torus_import = ModelCache::import("resources/torus.obj");
        
        RenderableComponent torus_renderable;
        for (std::string meshName : torus_import.meshNames)
        {
            torus_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        for (std::string matName : torus_import.materialNames)
        {
            torus_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(torus, torus_renderable);
        
        PhysicsComponent torus_physics;
        torus_physics.mass = 150.0f;
        //torus_physics.velocity = glm::vec3(1.0f, 0.0f, 0.0f);
        cosmos->add_component(torus, torus_physics);
        
        ColliderComponent torus_collider;
        torus_collider.colliders[1] = Collider(ColliderType::box, CollisionType::spring);
        torus_collider.colliders[1].localTransform.scale = glm::vec3(2.4f, 0.6f, 2.4f);
        torus_collider.colliders[1].inheritOrientation = true;
        torus_collider.colliders[1].restLength = 0.0f;
        torus_collider.colliders[1].stiffness  = 20000.0f;
        torus_collider.colliders[1].damping    = 400.0f;
        torus_collider.colliders[1].influenceOrientation = true;
        cosmos->add_component(torus, torus_collider);
        // ***************************************************************************

        Entity wall1 = cosmos->create_entity();
        cosmos->add_component(wall1, TransformComponent(glm::vec3(1.5f, 0.5f, -1.5f)));
        cosmos->get_component<TransformComponent>(wall1).orientation = glm::angleAxis(1.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        
        RenderableComponent wall1_renderable;
        wall1_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("wall1_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/toy_box_disp.png"}
        });
        wall1_renderable.materials.push_back(ModelCache::fetch_material("wall1_mat"));
        cosmos->add_component(wall1, wall1_renderable);
        
        Entity wall2 = cosmos->create_entity();
        cosmos->add_component(wall2, TransformComponent(glm::vec3(-1.0f, 1.0f, -1.0f)));
        cosmos->get_component<TransformComponent>(wall2).orientation = 
            glm::normalize(glm::angleAxis(1.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
            
        RenderableComponent wall2_renderable;
        wall2_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("wall2_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/spiral_disp.jpg"}
        });
        wall2_renderable.materials.push_back(ModelCache::fetch_material("wall2_mat"));
        cosmos->add_component(wall2, wall2_renderable);

        // ***************************************************************************
        Entity floor = cosmos->create_entity(false);
        cosmos->add_component(floor, TransformComponent(glm::vec3(0.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).scale = glm::vec3(20.0f, 20.0f, 0.05f);
        
        RenderableComponent floor_renderable;
        floor_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("floor_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/brickwall.jpg"},
            {TextureType::specular, "resources/brickwall_specular.jpg"},
            {TextureType::normal, "resources/brickwall_normal_up.jpg"},
        });
        floor_renderable.materials.push_back(ModelCache::fetch_material("floor_mat"));
        cosmos->add_component(floor, floor_renderable);


        PhysicsComponent floor_physics;
        // TODO: what mass to assign to non-dynamic objects? same as otherwise?
        // TODO: in general generate mass from known density
        floor_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        floor_physics.lockOrigin = true;
        floor_physics.lockedOrigin = cosmos->get_component<TransformComponent>(floor).origin;
        floor_physics.lockOrientation = true;
        floor_physics.lockedOrientation = cosmos->get_component<TransformComponent>(floor).orientation;
        cosmos->add_component(floor, floor_physics);
        cosmos->add_component(floor, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(floor).colliders[0].localTransform.origin.z = -0.499f;
        // ***************************************************************************

        // ***************************************************************************
        Entity snow = cosmos->create_entity(false);
        cosmos->add_component(snow, TransformComponent(glm::vec3(15.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).scale = glm::vec3(10.0f, 10.0f, 0.05f);

        RenderableComponent snow_renderable;
        snow_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("snow_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/snow-packed12-Base_Color.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal, "resources/snow-packed12-normal-ogl.png"},
            {TextureType::height, "resources/snow-packed12-Height.png"}
        });
        //snow_renderable.materials.push_back(ModelCache::fetch_material("snow_mat"));
        ModelCache::ImportReceipt snowy = ModelCache::import("./resources/snow_material.obj");
        snow_renderable.materials.push_back(ModelCache::fetch_material("snow_with_height"));
        cosmos->add_component(snow, snow_renderable);

        PhysicsComponent snow_physics;
        // TODO: what mass to assign to non-dynamic objects?
        // TODO: in general generate mass from known density
        snow_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        snow_physics.lockOrigin = true;
        snow_physics.lockedOrigin = cosmos->get_component<TransformComponent>(snow).origin;
        snow_physics.lockOrientation = true;
        snow_physics.lockedOrientation = cosmos->get_component<TransformComponent>(snow).orientation;
        cosmos->add_component(snow, snow_physics);
        cosmos->add_component(snow, ColliderComponent{
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(snow).colliders[0].localTransform.origin.z = -0.5f;
        // ***************************************************************************


        // ***************************************************************************
        // Scene needs to create an entity with camera component
        Entity mainCamera = cosmos->create_entity();
        cosmos->add_component(mainCamera, TransformComponent(glm::vec3(4.0f, 4.0f, 2.5f)));
        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        cosmos->add_component(mainCamera, mainCamera_physics);
        cosmos->get_component<TransformComponent>(mainCamera).orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, -0.7f));
        cosmos->add_component(mainCamera, CameraComponent());

        cosmos->add_component(mainCamera, SpacialInputComponent{});
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


        // ***************************************************************************
        Entity light = cosmos->create_entity();
        cosmos->add_component(light, TransformComponent(glm::vec3(0.0f, 1.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(light).scale = glm::vec3(0.2f);
        // remember this is relative to exposure
        cosmos->add_component(light, LightSourceComponent(glm::vec3(4.0f, 4.0f, 4.0f)));
        
        RenderableComponent light_renderable;
        light_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
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

        // RenderRelays should deal with each render phase
        //   and need to know camera entity's data
        // RenderRelays don't have access to Cosmos, so they need references to camera data
        // Cosmos builder could create relays for RenderDynamo, create entities and then assign
        //  how would it deal with dynamically created cameras...
        // Camera entities, like "rendered" entities need to have/know a relay type
        //   then the camera synchro can pass to dynamo and it would know what to do?

        return cosmos;
    }
}

#endif // TEST_COSMOS_H