#include "iceberg_cosmos.h"

#include "staging/cosmos_builder.h"
#include "logging/pleep_log.h"
#include "rendering/model_cache.h"
#include "behaviors/behaviors_library.h"
#include "staging/hard_config_cosmos.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_iceberg_cosmos(
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
        ModelCache::ImportReceipt nana_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - Super Smash Bros Melee - Ice Climbers\\Nana\\Nana.obj");
        //ModelCache::ImportReceipt nana_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - Super Smash Bros Melee - Ball Kirby Trophy\\Ball Kirby\\ballkirby.obj");
        ModelCache::ImportReceipt gamecube_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - Super Smash Bros Melee - GameCube\\Gamecube\\gamecube.obj");
        //ModelCache::ImportReceipt nana_import = ModelCache::import("..\\intercession_design\\adult-maloncremia-oot3dmm3d\\source\\3DS - The Legend of Zelda Majoras Mask 3D - Cremia\\cremia.dae");
        //ModelCache::ImportReceipt nana_import = ModelCache::import("C:/Users/Stephen/Repos/Intercession/resources/vampire/dancing_vampire3.dae");
        //ModelCache::ImportReceipt nana_import = ModelCache::import("./resources/12268_banjofrog_v1_L3.obj");
        
        //ModelManager::debug_receipt(nana_import);
        // ***************************************************************************

        // ***************************************************************************
        Entity nana = cosmos->create_entity();

        cosmos->add_component(nana, TransformComponent(glm::vec3(0.0f, -5.0f, 15.0f)));        cosmos->get_component<TransformComponent>(nana).orientation = glm::normalize(glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        
        RenderableComponent nana_renderable;
        nana_renderable.meshData = ModelCache::fetch_supermesh(nana_import.supermeshName);
        // load materials in order of submeshes
        for (const std::string matName : nana_import.supermeshMaterialNames)
        {
            nana_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(nana, nana_renderable);

        PhysicsComponent nana_physics;
        nana_physics.mass = 30.0f;
        //nana_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.2f);
        //cosmos->add_component(nana, nana_physics);


        // ***************************************************************************

        // ***************************************************************************
        Entity gamecube = cosmos->create_entity();
        cosmos->add_component(gamecube, TransformComponent(glm::vec3(-18.0f, -8.0f, 0.0f)));

        RenderableComponent gamecube_renderable;
        gamecube_renderable.meshData = ModelCache::fetch_supermesh(gamecube_import.supermeshName);
        // load materials in order of submeshes
        for (const std::string matName : gamecube_import.supermeshMaterialNames)
        {
            gamecube_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(gamecube, gamecube_renderable);

        PhysicsComponent gamecube_physics;
        gamecube_physics.mass = 5000;
        gamecube_physics.lockOrigin = true;
        gamecube_physics.lockedOrigin = cosmos->get_component<TransformComponent>(gamecube).origin;
        cosmos->add_component(gamecube, gamecube_physics);
        BoxColliderComponent gamecube_box;
        gamecube_box.localTransform.scale = glm::vec3(10.0f, 7.4f, 10.0f);
        gamecube_box.localTransform.origin = glm::vec3(0.0f, 3.8f, 0.0f);
        cosmos->add_component(gamecube, gamecube_box);
        cosmos->add_component(gamecube, RigidBodyComponent{});
        // ***************************************************************************


        // ***************************************************************************
        Entity crate = cosmos->create_entity();
        TransformComponent crateTransform(glm::vec3(2.9f, 0.0f, 0.3f));
        cosmos->add_component(crate, crateTransform);

        RenderableComponent crate_renderable;
        crate_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::cube);
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
        cosmos->add_component(crate, BoxColliderComponent{});
        cosmos->add_component(crate, RigidBodyComponent{});
        // ***************************************************************************


        // ***************************************************************************
        Entity block = cosmos->create_entity();

        cosmos->add_component(block, TransformComponent(glm::vec3(2.0f, -1.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).orientation = glm::normalize(glm::angleAxis(glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(block).scale = glm::vec3(1.8f, 0.3f, 1.8f);

        RenderableComponent block_renderable;
        block_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::cube);
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
        cosmos->add_component(block, BoxColliderComponent{});
        cosmos->add_component(block, RigidBodyComponent{});
        // ******************************************************************************

        Entity wall1 = cosmos->create_entity();
        cosmos->add_component(wall1, TransformComponent(glm::vec3(1.5f, 0.5f, -1.5f)));
        cosmos->get_component<TransformComponent>(wall1).orientation = glm::angleAxis(1.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        
        RenderableComponent wall1_renderable;
        wall1_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::quad);
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
        wall2_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::quad);
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
        floor_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::quad);
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
        cosmos->add_component(floor, BoxColliderComponent{});
        cosmos->get_component<BoxColliderComponent>(floor).localTransform.origin.z = -0.499f;
        cosmos->add_component(floor, RigidBodyComponent{});
        // ***************************************************************************

        // ***************************************************************************
        Entity snow = cosmos->create_entity(false);
        cosmos->add_component(snow, TransformComponent(glm::vec3(15.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).scale = glm::vec3(10.0f, 10.0f, 0.05f);

        RenderableComponent snow_renderable;
        snow_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::quad);
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
        cosmos->add_component(snow, BoxColliderComponent{});
        cosmos->get_component<BoxColliderComponent>(snow).localTransform.origin.z = -0.5f;
        cosmos->add_component(snow, RigidBodyComponent{});
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