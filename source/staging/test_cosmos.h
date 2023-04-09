#ifndef TEST_COSMOS_H
#define TEST_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "logging/pleep_log.h"
#include "rendering/model_library.h"
#include "scripting/script_library.h"
#include "core/cosmos_builder.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_test_cosmos(
        EventBroker* eventBroker, 
        RenderDynamo* renderDynamo, 
        InputDynamo* inputDynamo, 
        PhysicsDynamo* physicsDynamo, 
        I_NetworkDynamo* networkDynamo, 
        ScriptDynamo* scriptDynamo
    )
    {
        // TODO: receive config from server
        // TODO: Should config synchros just imply necessary components? Some are 1-to-1 like physics component, some are many-to-one like transform, some are unique like oscillator
        CosmosBuilder::Config cosmosConfig;
        cosmosConfig.components[0] = CosmosBuilder::ComponentType::transform;
        cosmosConfig.components[1] = CosmosBuilder::ComponentType::spacial_input;
        cosmosConfig.components[2] = CosmosBuilder::ComponentType::renderable;
        cosmosConfig.components[3] = CosmosBuilder::ComponentType::camera;
        cosmosConfig.components[4] = CosmosBuilder::ComponentType::light_source;
        cosmosConfig.components[5] = CosmosBuilder::ComponentType::physics;
        cosmosConfig.components[6] = CosmosBuilder::ComponentType::box_collider;
        cosmosConfig.components[7] = CosmosBuilder::ComponentType::ray_collider;
        cosmosConfig.components[8] = CosmosBuilder::ComponentType::rigid_body;
        cosmosConfig.components[9] = CosmosBuilder::ComponentType::spring_body;
        cosmosConfig.components[10] = CosmosBuilder::ComponentType::script;
        cosmosConfig.components[11] = CosmosBuilder::ComponentType::oscillator;

        cosmosConfig.synchros[0] = CosmosBuilder::SynchroType::spacial_input;
        cosmosConfig.synchros[1] = CosmosBuilder::SynchroType::lighting;
        cosmosConfig.synchros[2] = CosmosBuilder::SynchroType::render;
        cosmosConfig.synchros[3] = CosmosBuilder::SynchroType::physics;
        cosmosConfig.synchros[4] = CosmosBuilder::SynchroType::box_collider;
        cosmosConfig.synchros[5] = CosmosBuilder::SynchroType::ray_collider;
        cosmosConfig.synchros[6] = CosmosBuilder::SynchroType::network;
        cosmosConfig.synchros[7] = CosmosBuilder::SynchroType::script;

        // build cosmos according to config
        CosmosBuilder generator;
        std::shared_ptr<Cosmos> cosmos = generator.generate(cosmosConfig, NULL_TIMESLICEID , eventBroker, renderDynamo, inputDynamo, physicsDynamo, networkDynamo, scriptDynamo);

        //PLEEPLOG_DEBUG(cosmos->stringify_synchro_registry());

        PLEEPLOG_TRACE("Create Entities");
        // create entities
        // create component and pass or construct inline
        // if component is explicit (no initalizer list), we can omit template

        // ***************************************************************************
        ModelLibrary::ImportReceipt frog_import = ModelLibrary::import("..\\intercession_design\\adult-maloncremia-oot3dmm3d\\source\\3DS - The Legend of Zelda Majoras Mask 3D - Cremia\\cremia.dae");
        //ModelLibrary::import("C:/Users/Stephen/Repos/Intercession/resources/vampire/dancing_vampire3.dae");
        //ModelLibrary::import("./resources/12268_banjofrog_v1_L3.obj");
        //return;
        // ***************************************************************************

        // ***************************************************************************
        Entity frog = cosmos->create_entity();

        //cosmos->add_component(frog, MetaComponent{ "froog" });
        cosmos->add_component(frog, TransformComponent(glm::vec3(6.0f, 2.0f, -0.5f)));
        //cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.001f, 0.001f, 0.001f);
        
        RenderableComponent frog_renderable;
        frog_renderable.meshData = ModelLibrary::fetch_supermesh(frog_import.supermeshNames[0]);
        for (std::string matName : frog_import.supermeshMaterialsNames[0])
        {
            frog_renderable.materials.push_back(ModelLibrary::fetch_material(matName));
        }
        //std::shared_ptr<Model> frog_model = model_builder::create_cube("resources/container.jpg");
        cosmos->add_component(frog, frog_renderable);
        PhysicsComponent frog_physics;
        frog_physics.mass = 30.0f;
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
        crate_renderable.meshData = ModelLibrary::fetch_cube_supermesh();
        ModelLibrary::create_material("crate_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/container2.png"},
            {TextureType::specular, "resources/container2_specular.png"}
        });
        crate_renderable.materials.push_back(ModelLibrary::fetch_material("crate_mat"));
        cosmos->add_component(crate, crate_renderable);

        PhysicsComponent crate_physics;
        //crate_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        crate_physics.angularVelocity = glm::vec3(0.0f, 0.7f, 0.2f);
        crate_physics.mass = 100.0f;
        cosmos->add_component(crate, crate_physics);
        cosmos->add_component(crate, BoxColliderComponent{});
        cosmos->add_component(crate, RigidBodyComponent{});
        // ***************************************************************************

        // ***************************************************************************
        Entity block = cosmos->create_entity();

        cosmos->add_component(block, TransformComponent(glm::vec3(2.0f, -1.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).orientation = glm::normalize(glm::angleAxis(glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).scale = glm::vec3(1.8f, 0.3f, 1.8f);

        RenderableComponent block_renderable;
        block_renderable.meshData = ModelLibrary::fetch_cube_supermesh();
        ModelLibrary::create_material("block_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/bricks2.jpg"},
            {TextureType::specular, "resources/bricks2_disp.jpg"},
            {TextureType::normal, "resources/bricks2_normal.jpg"}
        });
        //cosmos->add_component(block, ModelComponent(model_builder::create_cube("resources/bricks2.jpg", "resources/bricks2_disp.jpg", "resources/bricks2_normal.jpg")));
        block_renderable.materials.push_back(ModelLibrary::fetch_material("block_mat"));
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
        cosmos->add_component(block, BoxColliderComponent{});
        cosmos->add_component(block, RigidBodyComponent{});
        // ******************************************************************************

        // ***************************************************************************
        Entity torus = cosmos->create_entity();

        TransformComponent torus_transform;
        torus_transform.origin = glm::vec3(0.3f, 1.0f, 0.0f);
        torus_transform.orientation = glm::angleAxis(glm::radians(30.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        //torus_transform.scale = glm::vec3(1.0f, 1.35f, 0.75f);
        cosmos->add_component(torus, torus_transform);
        ModelLibrary::ImportReceipt torus_import = ModelLibrary::import("resources/torus.obj");
        
        RenderableComponent torus_renderable;
        torus_renderable.meshData = ModelLibrary::fetch_supermesh(torus_import.supermeshNames[0]);
        for (std::string matName : torus_import.supermeshMaterialsNames[0])
        {
            torus_renderable.materials.push_back(ModelLibrary::fetch_material(matName));
        }
        cosmos->add_component(torus, torus_renderable);
        
        PhysicsComponent torus_physics;
        torus_physics.mass = 150.0f;
        //torus_physics.velocity = glm::vec3(1.0f, 0.0f, 0.0f);
        cosmos->add_component(torus, torus_physics);
        
        BoxColliderComponent torus_collider;
        torus_collider.localTransform.scale = glm::vec3(2.4f, 0.6f, 2.4f);
        torus_collider.responseType = CollisionResponseType::spring;
        torus_collider.inheritOrientation = true;
        cosmos->add_component(torus, torus_collider);
        
        SpringBodyComponent torus_springBody;
        torus_springBody.restLength = 0.0f;
        torus_springBody.stiffness  = 20000.0f;
        torus_springBody.damping    = 400.0f;
        torus_springBody.influenceOrientation = true;
        cosmos->add_component(torus, torus_springBody);
        // ***************************************************************************

        Entity wall1 = cosmos->create_entity();
        cosmos->add_component(wall1, TransformComponent(glm::vec3(1.5f, 0.5f, -1.5f)));
        cosmos->get_component<TransformComponent>(wall1).orientation = glm::angleAxis(1.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        
        RenderableComponent wall1_renderable;
        wall1_renderable.meshData = ModelLibrary::fetch_quad_supermesh();
        ModelLibrary::create_material("wall1_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/toy_box_disp.png"}
        });
        wall1_renderable.materials.push_back(ModelLibrary::fetch_material("wall1_mat"));
        cosmos->add_component(wall1, wall1_renderable);
        
        Entity wall2 = cosmos->create_entity();
        cosmos->add_component(wall2, TransformComponent(glm::vec3(-1.0f, 1.0f, -1.0f)));
        cosmos->get_component<TransformComponent>(wall2).orientation = 
            glm::normalize(glm::angleAxis(1.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
            
        RenderableComponent wall2_renderable;
        wall2_renderable.meshData = ModelLibrary::fetch_quad_supermesh();
        ModelLibrary::create_material("wall2_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/spiral_disp.jpg"}
        });
        wall2_renderable.materials.push_back(ModelLibrary::fetch_material("wall2_mat"));
        cosmos->add_component(wall2, wall2_renderable);

        // ***************************************************************************
        Entity floor = cosmos->create_entity();
        cosmos->add_component(floor, TransformComponent(glm::vec3(0.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).scale = glm::vec3(10.0f, 10.0f, 0.05f);
        
        RenderableComponent floor_renderable;
        floor_renderable.meshData = ModelLibrary::fetch_quad_supermesh();
        ModelLibrary::create_material("floor_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/brickwall.jpg"},
            {TextureType::specular, "resources/brickwall_specular.jpg"},
            {TextureType::normal, "resources/brickwall_normal_up.jpg"},
        });
        //ModelLibrary::create_material("floor_mat", ...);
        floor_renderable.materials.push_back(ModelLibrary::fetch_material("floor_mat"));
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
        cosmos->add_component(floor, BoxColliderComponent{});
        cosmos->get_component<BoxColliderComponent>(floor).localTransform.origin.z = -0.499f;
        cosmos->add_component(floor, RigidBodyComponent{});
        // ***************************************************************************

        // ***************************************************************************
        Entity snow = cosmos->create_entity();
        cosmos->add_component(snow, TransformComponent(glm::vec3(10.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).scale = glm::vec3(10.0f, 10.0f, 0.05f);

        RenderableComponent snow_renderable;
        snow_renderable.meshData = ModelLibrary::fetch_quad_supermesh();
        ModelLibrary::create_material("snow_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/snow-packed12-Base_Color.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal, "resources/snow-packed12-normal-ogl.png"},
            {TextureType::height, "resources/snow-packed12-Height.png"}
        });
        snow_renderable.materials.push_back(ModelLibrary::fetch_material("snow_mat"));
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
        cosmos->add_component(snow, BoxColliderComponent{});
        cosmos->get_component<BoxColliderComponent>(snow).localTransform.origin.z = -0.5f;
        cosmos->add_component(snow, RigidBodyComponent{});
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


        // ***************************************************************************
        Entity light = cosmos->create_entity();
        cosmos->add_component(light, TransformComponent(glm::vec3(0.0f, 1.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(light).scale = glm::vec3(0.2f);
        // remember this is relative to exposure
        cosmos->add_component(light, LightSourceComponent(glm::vec3(4.0f, 4.0f, 4.0f)));
        
        RenderableComponent light_renderable;
        light_renderable.meshData = ModelLibrary::fetch_icosahedron_supermesh();
        ModelLibrary::create_material("lightbulb_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse,  "resources/blending_transparent_window.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
            //{TextureType::height,   "resources/snow-packed12-Height.png"},
            {TextureType::emissive, "resources/snow-packed12-Specular.png"}
        });
        light_renderable.materials.push_back(ModelLibrary::fetch_material("lightbulb_mat"));
        cosmos->add_component(light, light_renderable);
        
        ScriptComponent light_scripts;
        light_scripts.drivetrain = ScriptLibrary::fetch_script(ScriptLibrary::ScriptType::oscillator);
        light_scripts.use_fixed_update = true;
        cosmos->add_component(light, light_scripts);
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