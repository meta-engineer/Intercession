#ifndef TEST_COSMOS_H
#define TEST_COSMOS_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"
#include "rendering/model_library.h"

// TODO: This is temporary for building hard-coded entities
#include "inputting/spacial_input_synchro.h"
#include "rendering/render_synchro.h"
#include "rendering/lighting_synchro.h"
#include "physics/physics_synchro.h"
#include "physics/box_collider_synchro.h"
#include "physics/ray_collider_synchro.h"
#include "networking/network_synchro.h"
#include "scripting/script_synchro.h"

#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/box_collider_component.h"
#include "physics/ray_collider_component.h"
#include "physics/rigid_body_component.h"
#include "inputting/spacial_input_component.h"
#include "rendering/renderable_component.h"
#include "rendering/camera_component.h"
#include "rendering/light_source_component.h"
#include "core/meta_component.h"

#include "scripting/script_component.h"
#include "scripting/biped_scripts.h"
#include "scripting/fly_control_scripts.h"
#include "scripting/oscillator_scripts.h"
#include "scripting/oscillator_component.h"

namespace pleep
{
    void build_test_cosmos(
        Cosmos* cosmos, 
        EventBroker* eventBroker, 
        RenderDynamo* renderDynamo, 
        InputDynamo* inputDynamo, 
        PhysicsDynamo* physicsDynamo, 
        I_NetworkDynamo* networkDynamo, 
        ScriptDynamo* scriptDynamo
    )
    {
        // Assume Cosmos is already "empty"

        // register components
        cosmos->register_component<TransformComponent>();
        cosmos->register_component<SpacialInputComponent>();
        cosmos->register_component<RenderableComponent>();
        cosmos->register_component<CameraComponent>();
        cosmos->register_component<LightSourceComponent>();
        cosmos->register_component<PhysicsComponent>();
        cosmos->register_component<BoxColliderComponent>();
        cosmos->register_component<RayColliderComponent>();
        cosmos->register_component<RigidBodyComponent>();
        cosmos->register_component<SpringBodyComponent>();
        cosmos->register_component<ScriptComponent>();
        cosmos->register_component<OscillatorComponent>();
        // We may want to enforce use of meta component for all cosmos'?
        //cosmos->register_component<MetaComponent>();

        ScriptLibrary::clear_library();
        ScriptLibrary::register_script<BipedScripts>();
        ScriptLibrary::register_script<FlyControlScripts>();

        // register/create synchros, set component signatures
        // we shouldn't need to keep synchro references after we config them here, 
        // we'll only access through Cosmos
        // any other functionallity should be in dynamos
        PLEEPLOG_TRACE("Create Synchros");

        std::shared_ptr<SpacialInputSynchro> spacialInputSynchro = cosmos->register_synchro<SpacialInputSynchro>();
        {
            spacialInputSynchro->attach_dynamo(inputDynamo);
            cosmos->set_synchro_signature<SpacialInputSynchro>(SpacialInputSynchro::get_signature(cosmos));
        }

        // synchros are in an unordered map so it isn't guarenteed that LightingSynchro is invoked before RenderSynchro
        // TODO: ordering of synchros in unordered_map DOES AFFECT run order, with undefined, NON-DETERMINISTIC behaviour
        std::shared_ptr<LightingSynchro> lightingSynchro = cosmos->register_synchro<LightingSynchro>();
        {
            lightingSynchro->attach_dynamo(renderDynamo);
            cosmos->set_synchro_signature<LightingSynchro>(LightingSynchro::get_signature(cosmos));
        }

        std::shared_ptr<RenderSynchro> renderSynchro = cosmos->register_synchro<RenderSynchro>();
        {
            renderSynchro->attach_dynamo(renderDynamo);
            cosmos->set_synchro_signature<RenderSynchro>(RenderSynchro::get_signature(cosmos));
        }

        // TODO: maybe specify this is "motion integration" not just all physics
        std::shared_ptr<PhysicsSynchro> physicsSynchro = cosmos->register_synchro<PhysicsSynchro>();
        {
            physicsSynchro->attach_dynamo(physicsDynamo);
            cosmos->set_synchro_signature<PhysicsSynchro>(PhysicsSynchro::get_signature(cosmos));
        }

        std::shared_ptr<BoxColliderSynchro> boxColliderSynchro = cosmos->register_synchro<BoxColliderSynchro>();
        {
            boxColliderSynchro->attach_dynamo(physicsDynamo);
            cosmos->set_synchro_signature<BoxColliderSynchro>(BoxColliderSynchro::get_signature(cosmos));
        }
        
        std::shared_ptr<RayColliderSynchro> rayColliderSynchro = cosmos->register_synchro<RayColliderSynchro>();
        {
            rayColliderSynchro->attach_dynamo(physicsDynamo);
            cosmos->set_synchro_signature<RayColliderSynchro>(RayColliderSynchro::get_signature(cosmos));
        }

        std::shared_ptr<NetworkSynchro> networkSynchro = cosmos->register_synchro<NetworkSynchro>();
        {
            networkSynchro->attach_dynamo(networkDynamo);
            cosmos->set_synchro_signature<NetworkSynchro>(NetworkSynchro::get_signature(cosmos));
        }

        std::shared_ptr<ScriptSynchro> scriptSynchro = cosmos->register_synchro<ScriptSynchro>();
        {
            scriptSynchro->attach_dynamo(scriptDynamo);
            cosmos->set_synchro_signature<ScriptSynchro>(ScriptSynchro::get_signature(cosmos));
        }

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
        Entity frog = cosmos->create_local_entity();

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
        frog_scripts.drivetrain = ScriptLibrary::fetch_scripts<BipedScripts>();
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
        Entity vamp = cosmos->create_local_entity();
        cosmos->add_component(vamp, TransformComponent(glm::vec3(2.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(vamp).scale = glm::vec3(0.01f, 0.01f, 0.01f);
        std::shared_ptr<Model> vampModel = std::make_shared<Model>("resources/vampire/dancing_vampire3.dae");
        cosmos->add_component(vamp, ModelComponent(vampModel));
*/

        // ***************************************************************************
        Entity crate = cosmos->create_local_entity();
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
        Entity block = cosmos->create_local_entity();

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
        Entity torus = cosmos->create_local_entity();

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

        Entity wall1 = cosmos->create_local_entity();
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
        
        Entity wall2 = cosmos->create_local_entity();
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
        Entity floor = cosmos->create_local_entity();
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
        Entity snow = cosmos->create_local_entity();
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
        Entity mainCamera = cosmos->create_local_entity();
        cosmos->add_component(mainCamera, TransformComponent(glm::vec3(5.0f, 2.5f, 6.0f)));
        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        cosmos->add_component(mainCamera, mainCamera_physics);
        cosmos->get_component<TransformComponent>(mainCamera).orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, -0.2f));
        cosmos->add_component(mainCamera, CameraComponent());

        cosmos->add_component(mainCamera, SpacialInputComponent());
        ScriptComponent camera_scripts;
        camera_scripts.drivetrain = ScriptLibrary::fetch_scripts<FlyControlScripts>();
        camera_scripts.use_fixed_update = true;
        cosmos->add_component(mainCamera, camera_scripts);

        // then it needs to be assigned somewhere in render pipeline (view camera, shadow camera, etc)
        // assuming there is only ever 1 main camera we can notify over event broker
        // dynamos don't have acess to cosmos, so they can't lookup entity
        // synchro can maintain camera and pass its data each frame
        
        EventMessage cameraEvent(events::rendering::SET_MAIN_CAMERA);
        events::rendering::SET_MAIN_CAMERA_params cameraParams {
            mainCamera
        };
        cameraEvent << cameraParams;
        
        // TODO: unit testing
        renderSynchro->attach_dynamo(nullptr);
        eventBroker->send_event(cameraEvent);
        renderSynchro->attach_dynamo(renderDynamo);

        eventBroker->send_event(cameraEvent);
        // ***************************************************************************


        // ***************************************************************************
        Entity light = cosmos->create_local_entity();
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
        light_scripts.drivetrain = std::make_shared<OscillatorScripts>();
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

    }
}

#endif // TEST_COSMOS_H