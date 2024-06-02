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

        cosmos->add_component(nana, TransformComponent(glm::vec3(-20.0f, 0.0f, 20.0f)));        cosmos->get_component<TransformComponent>(nana).orientation = glm::normalize(glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        
        RenderableComponent nana_renderable;
        for (const std::string meshName : nana_import.meshNames)
        {
            nana_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        // load materials in order of meshes
        for (const std::string matName : nana_import.materialNames)
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
        cosmos->add_component(gamecube, TransformComponent(glm::vec3(-20.0f, 0.0f, -20.0f)));

        RenderableComponent gamecube_renderable;
        for (const std::string meshName : gamecube_import.meshNames)
        {
            gamecube_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        // load materials in order of meshes
        for (const std::string matName : gamecube_import.materialNames)
        {
            gamecube_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(gamecube, gamecube_renderable);

        PhysicsComponent gamecube_physics;
        gamecube_physics.mass = 5000;
        gamecube_physics.lockOrigin = true;
        gamecube_physics.lockedOrigin = cosmos->get_component<TransformComponent>(gamecube).origin;
        cosmos->add_component(gamecube, gamecube_physics);
        ColliderComponent gamecube_collider{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        };
        gamecube_collider.colliders[0].localTransform.scale = glm::vec3(10.0f, 7.4f, 10.0f);
        gamecube_collider.colliders[0].localTransform.origin = glm::vec3(0.0f, 3.8f, 0.0f);
        cosmos->add_component(gamecube, gamecube_collider);
        // ***************************************************************************

        
        // ***************************************************************************
        Entity gamebox = cosmos->create_entity();

        TransformComponent gamebox_transform(glm::vec3(-1.0f, 0.0f, -20.0f));
        gamebox_transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->add_component(gamebox, gamebox_transform);

        RenderableComponent gamebox_renderable;
        for (const std::string meshName : gamecube_import.meshNames)
        {
            gamebox_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        // load materials in order of meshes
        for (const std::string matName : gamecube_import.materialNames)
        {
            gamebox_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(gamebox, gamebox_renderable);

        PhysicsComponent gamebox_physics;
        gamebox_physics.mass = 200.0f;
        cosmos->add_component(gamebox, gamebox_physics);

        ColliderComponent gamebox_collider{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        };
        gamebox_collider.colliders[0].dynamicFriction = 0.8f;
        gamebox_collider.colliders[0].restitution = 0.2f;
        gamebox_collider.colliders[0].localTransform.scale = glm::vec3(10.0f, 7.4f, 10.0f);
        cosmos->add_component(gamebox, gamebox_collider);
        // ***************************************************************************


        // ***************************************************************************
        Entity crate = cosmos->create_entity();
        TransformComponent crateTransform(glm::vec3(2.9f, 0.0f, -20.3f));
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


        // ***************************************************************************
        Entity block = cosmos->create_entity();

        cosmos->add_component(block, TransformComponent(glm::vec3(2.0f, -1.0f, -20.0f)));
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
        ModelCache::create_material("floor_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/moon_diffuse.png"},
            {TextureType::specular, "resources/moon_diffuse.png"},
            {TextureType::normal, "resources/moon_normal.png"},
        });
        for (int x = -1; x < 2; x++)
        {
            for (int z = -1; z < 2; z++)
            {
                Entity floor = cosmos->create_entity(false);
                cosmos->add_component(floor, TransformComponent(glm::vec3(x*20.0f, -2.0f, z*20.0f)));
                cosmos->get_component<TransformComponent>(floor).orientation = 
                    glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
                cosmos->get_component<TransformComponent>(floor).scale = glm::vec3(20.0f, 20.0f, 0.05f);
                
                RenderableComponent floor_renderable;
                floor_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
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
            }
        }

        // ***************************************************************************

        // ***************************************************************************
        Entity snow = cosmos->create_entity(false); // # 14
        cosmos->add_component(snow, TransformComponent(glm::vec3(20.0f, -1.0f, 20.0f)));
        cosmos->get_component<TransformComponent>(snow).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(snow).scale = glm::vec3(20.0f, 20.0f, 1.0f);

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
        // ***************************************************************************

        // ******************************************************************************
        Entity panel1 = cosmos->create_entity();
        cosmos->add_component(panel1, TransformComponent(glm::vec3(21.0f, 0.5f, 22.0f)));
        cosmos->get_component<TransformComponent>(panel1).orientation = glm::angleAxis(3.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        
        RenderableComponent panel1_renderable;
        panel1_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("panel1_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/toy_box_disp.png"}
        });
        panel1_renderable.materials.push_back(ModelCache::fetch_material("panel1_mat"));
        cosmos->add_component(panel1, panel1_renderable);
        
        Entity panel2 = cosmos->create_entity();
        cosmos->add_component(panel2, TransformComponent(glm::vec3(19.0f, 1.0f, 22.0f)));
        cosmos->get_component<TransformComponent>(panel2).orientation = 
            glm::normalize(glm::angleAxis(2.5f, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        RenderableComponent panel2_renderable;
        panel2_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        ModelCache::create_material("panel2_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/wood.png"},
            {TextureType::specular, "resources/wood.png"},
            {TextureType::normal, "resources/toy_box_normal.png"},
            {TextureType::height, "resources/spiral_disp.jpg"}
        });
        panel2_renderable.materials.push_back(ModelCache::fetch_material("panel2_mat"));
        cosmos->add_component(panel2, panel2_renderable);
        // ******************************************************************************


        // ******************************************************************************
        Entity ramp1 = cosmos->create_entity(false);
        cosmos->add_component(ramp1, TransformComponent(glm::vec3(20.0f, -1.0f, 3.0f)));
        cosmos->get_component<TransformComponent>(ramp1).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(ramp1).scale = glm::vec3(10.0f, 0.5f, 2.0f);

        RenderableComponent ramp1_renderable;
        ramp1_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        ramp1_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(ramp1, ramp1_renderable);

        PhysicsComponent ramp1_physics;
        ramp1_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        ramp1_physics.lockOrigin = true;
        ramp1_physics.lockedOrigin = cosmos->get_component<TransformComponent>(ramp1).origin;
        ramp1_physics.lockOrientation = true;
        ramp1_physics.lockedOrientation = cosmos->get_component<TransformComponent>(ramp1).orientation;
        cosmos->add_component(ramp1, ramp1_physics);
        cosmos->add_component(ramp1, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        
        Entity ramp2 = cosmos->create_entity(false);
        cosmos->add_component(ramp2, TransformComponent(glm::vec3(20.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(ramp2).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(ramp2).scale = glm::vec3(10.0f, 0.5f, 2.0f);

        RenderableComponent ramp2_renderable;
        ramp2_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        ramp2_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(ramp2, ramp2_renderable);

        PhysicsComponent ramp2_physics;
        ramp2_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        ramp2_physics.lockOrigin = true;
        ramp2_physics.lockedOrigin = cosmos->get_component<TransformComponent>(ramp2).origin;
        ramp2_physics.lockOrientation = true;
        ramp2_physics.lockedOrientation = cosmos->get_component<TransformComponent>(ramp2).orientation;
        cosmos->add_component(ramp2, ramp2_physics);
        cosmos->add_component(ramp2, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        
        Entity ramp3 = cosmos->create_entity(false);
        cosmos->add_component(ramp3, TransformComponent(glm::vec3(20.0f, 1.0f, -3.0f)));
        cosmos->get_component<TransformComponent>(ramp3).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(75.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        cosmos->get_component<TransformComponent>(ramp3).scale = glm::vec3(10.0f, 0.5f, 2.0f);

        RenderableComponent ramp3_renderable;
        ramp3_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        ramp3_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(ramp3, ramp3_renderable);

        PhysicsComponent ramp3_physics;
        ramp3_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        ramp3_physics.lockOrigin = true;
        ramp3_physics.lockedOrigin = cosmos->get_component<TransformComponent>(ramp3).origin;
        ramp3_physics.lockOrientation = true;
        ramp3_physics.lockedOrientation = cosmos->get_component<TransformComponent>(ramp3).orientation;
        cosmos->add_component(ramp3, ramp3_physics);
        cosmos->add_component(ramp3, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        // ******************************************************************************

        
        // ******************************************************************************
        Entity teeter = cosmos->create_entity();
        cosmos->add_component(teeter, TransformComponent(glm::vec3(20.0f, 0.0f, -20.0f)));
        cosmos->get_component<TransformComponent>(teeter).scale = glm::vec3(10.0f, 0.5f, 5.0f);

        RenderableComponent teeter_renderable;
        teeter_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        teeter_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(teeter, teeter_renderable);

        PhysicsComponent teeter_physics;
        teeter_physics.mass = 3000.0f;
        teeter_physics.lockOrigin = true;
        teeter_physics.lockedOrigin = cosmos->get_component<TransformComponent>(teeter).origin;
        cosmos->add_component(teeter, teeter_physics);
        cosmos->add_component(teeter, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        // ******************************************************************************


        // ***************************************************************************
        Entity wall1 = cosmos->create_entity(false);
        cosmos->add_component(wall1, TransformComponent(glm::vec3(30.0f, 8.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(wall1).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(wall1).scale = glm::vec3(60.0f, 20.0f, 1.0f);

        RenderableComponent wall1_renderable;
        wall1_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        wall1_renderable.materials.push_back(ModelCache::fetch_material("block_mat"));
        cosmos->add_component(wall1, wall1_renderable);

        PhysicsComponent wall1_physics;
        wall1_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        wall1_physics.lockOrigin = true;
        wall1_physics.lockedOrigin = cosmos->get_component<TransformComponent>(wall1).origin;
        wall1_physics.lockOrientation = true;
        wall1_physics.lockedOrientation = cosmos->get_component<TransformComponent>(wall1).orientation;
        cosmos->add_component(wall1, wall1_physics);
        cosmos->add_component(wall1, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(wall1).colliders[0].localTransform.origin.z = -0.499f;

        Entity wall2 = cosmos->create_entity(false);
        cosmos->add_component(wall2, TransformComponent(glm::vec3(0.0f, 8.0f, -30.0f)));
        cosmos->get_component<TransformComponent>(wall2).scale = glm::vec3(60.0f, 20.0f, 1.0f);

        RenderableComponent wall2_renderable;
        wall2_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        wall2_renderable.materials.push_back(ModelCache::fetch_material(*(gamecube_import.materialNames.begin())));
        cosmos->add_component(wall2, wall2_renderable);

        PhysicsComponent wall2_physics;
        wall2_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        wall2_physics.lockOrigin = true;
        wall2_physics.lockedOrigin = cosmos->get_component<TransformComponent>(wall2).origin;
        wall2_physics.lockOrientation = true;
        wall2_physics.lockedOrientation = cosmos->get_component<TransformComponent>(wall2).orientation;
        cosmos->add_component(wall2, wall2_physics);
        cosmos->add_component(wall2, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(wall2).colliders[0].localTransform.origin.z = -0.499f;

        Entity wall3 = cosmos->create_entity(false);
        cosmos->add_component(wall3, TransformComponent(glm::vec3(-30.0f, 8.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(wall3).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(wall3).scale = glm::vec3(60.0f, 20.0f, 1.0f);

        RenderableComponent wall3_renderable;
        wall3_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        wall3_renderable.materials.push_back(ModelCache::fetch_material(*(nana_import.materialNames.begin())));
        cosmos->add_component(wall3, wall3_renderable);

        PhysicsComponent wall3_physics;
        wall3_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        wall3_physics.lockOrigin = true;
        wall3_physics.lockedOrigin = cosmos->get_component<TransformComponent>(wall3).origin;
        wall3_physics.lockOrientation = true;
        wall3_physics.lockedOrientation = cosmos->get_component<TransformComponent>(wall3).orientation;
        cosmos->add_component(wall3, wall3_physics);
        cosmos->add_component(wall3, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(wall3).colliders[0].localTransform.origin.z = -0.499f;
        // ***************************************************************************


        // ***************************************************************************
        Entity light = cosmos->create_entity();
        cosmos->add_component(light, TransformComponent(glm::vec3(20.0f, 1.0f, 20.0f)));
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


        // ***************************************************************************
        Entity spot = cosmos->create_entity();
        cosmos->add_component(spot, TransformComponent(glm::vec3(0.0f, 4.0f, -20.0f)));
        cosmos->get_component<TransformComponent>(spot).scale = glm::vec3(0.2f);
        cosmos->get_component<TransformComponent>(spot).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        // remember this is relative to exposure
        cosmos->add_component(spot, LightSourceComponent(glm::vec3(2.0f, 3.0f, 6.0f)));
        cosmos->get_component<LightSourceComponent>(spot).type = LightSourceType::spot;
        cosmos->get_component<LightSourceComponent>(spot).attributes = glm::vec2{0.90f, 0.70f};

        RenderableComponent spot_renderable;
        spot_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
        spot_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(spot, spot_renderable);
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