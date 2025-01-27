#include "moon_cosmos.h"

#include "staging/cosmos_builder.h"
#include "logging/pleep_log.h"
#include "rendering/model_cache.h"
#include "behaviors/behaviors_library.h"
#include "staging/hard_config_cosmos.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_moon_cosmos(
        std::shared_ptr<EventBroker> eventBroker, 
        DynamoCluster& dynamoCluster
    )
    {
        std::shared_ptr<Cosmos> cosmos = construct_hard_config_cosmos(eventBroker, dynamoCluster);
        
        // ***************************************************************************
        ModelCache::create_material("floor_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/grid.png"},
            {TextureType::specular, "resources/grid.png"},
            {TextureType::normal, "resources/grid_normal.png"},
        });

        Entity floor_1 = cosmos->create_entity(false);
        cosmos->add_component(floor_1, TransformComponent(glm::vec3(12.0f, -4.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor_1).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor_1).scale = glm::vec3(24.0f, 24.0f, 2.0f);
        
        RenderableComponent floor_1_renderable;
        floor_1_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        floor_1_renderable.materials.push_back(ModelCache::fetch_material("floor_mat"));
        cosmos->add_component(floor_1, floor_1_renderable);
        
        PhysicsComponent floor_1_physics;
        // TODO: in general generate mass from a known density
        floor_1_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        floor_1_physics.lockOrigin = true;
        floor_1_physics.lockedOrigin = cosmos->get_component<TransformComponent>(floor_1).origin;
        floor_1_physics.lockOrientation = true;
        floor_1_physics.lockedOrientation = cosmos->get_component<TransformComponent>(floor_1).orientation;
        cosmos->add_component(floor_1, floor_1_physics);
        cosmos->add_component(floor_1, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(floor_1).colliders[0].localTransform.origin.z = -0.499f;

        
        Entity floor_2 = cosmos->create_entity(false);
        cosmos->add_component(floor_2, TransformComponent(glm::vec3(-12.0f, -6.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor_2).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor_2).scale = glm::vec3(24.0f, 24.0f, 0.05f);
        
        RenderableComponent floor_2_renderable;
        floor_2_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::quad));
        floor_2_renderable.materials.push_back(ModelCache::fetch_material("floor_mat"));
        cosmos->add_component(floor_2, floor_2_renderable);
        
        PhysicsComponent floor_2_physics;
        // TODO: in general generate mass from a known density
        floor_2_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        floor_2_physics.lockOrigin = true;
        floor_2_physics.lockedOrigin = cosmos->get_component<TransformComponent>(floor_2).origin;
        floor_2_physics.lockOrientation = true;
        floor_2_physics.lockedOrientation = cosmos->get_component<TransformComponent>(floor_2).orientation;
        cosmos->add_component(floor_2, floor_2_physics);
        cosmos->add_component(floor_2, ColliderComponent{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        });
        cosmos->get_component<ColliderComponent>(floor_2).colliders[0].localTransform.origin.z = -0.499f;
        // ***************************************************************************

        /***************************************************************************
        ModelCache::ImportReceipt gamecube_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - Super Smash Bros Melee - GameCube\\Gamecube\\gamecube.obj");

        Entity gamebox = cosmos->create_entity();

        TransformComponent gamebox_transform(glm::vec3(-10.0f, 1.0f, 0.0f));
        gamebox_transform.scale = {2.0f, 2.0f, 2.0f};
        cosmos->add_component(gamebox, gamebox_transform);

        RenderableComponent gamebox_renderable;
        for (const std::string matName : gamecube_import.meshNames)
        {
            gamebox_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        }
        // load materials in order of meshes
        for (const std::string matName : gamecube_import.materialNames)
        {
            gamebox_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        gamebox_renderable.localTransform.origin.y = -0.375f;
        gamebox_renderable.localTransform.scale = { 0.1f, 0.1f, 0.1f};
        cosmos->add_component(gamebox, gamebox_renderable);

        PhysicsComponent gamebox_physics;
        gamebox_physics.mass = 100.0f;
        cosmos->add_component(gamebox, gamebox_physics);
        
        ColliderComponent gamebox_collider{ 
            { Collider(ColliderType::box, CollisionType::rigid) }
        };
        gamebox_collider.colliders[0].localTransform.scale = glm::vec3(1.0f, 0.75f, 1.0f);
        gamebox_collider.colliders[0].dynamicFriction = 0.9f;
        gamebox_collider.colliders[0].restitution = 0.1f;
        cosmos->add_component(gamebox, gamebox_collider);
        // ***************************************************************************/
        
        //**************************************************************************
        ModelCache::create_material("lightbulb_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse,  "resources/blending_transparent_window.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
            //{TextureType::height,   "resources/snow-packed12-Height.png"},
            {TextureType::emissive, "resources/snow-packed12-Specular.png"}
        });

        Entity spot = cosmos->create_entity();
        cosmos->add_component(spot, TransformComponent(glm::vec3(-12.0f, 4.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(spot).scale = glm::vec3(0.2f);
        cosmos->get_component<TransformComponent>(spot).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        // remember this is relative to exposure
        cosmos->add_component(spot, LightSourceComponent(glm::vec3(6.0f, 3.0f, 2.0f)));
        cosmos->get_component<LightSourceComponent>(spot).type = LightSourceType::spot;
        cosmos->get_component<LightSourceComponent>(spot).composition.x = 0.2f;
        cosmos->get_component<LightSourceComponent>(spot).attributes = glm::vec2{0.90f, 0.70f};

        RenderableComponent spot_renderable;
        spot_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
        spot_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(spot, spot_renderable);
        

        Entity spottier = cosmos->create_entity();
        cosmos->add_component(spottier, TransformComponent(glm::vec3(12.0f, 4.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(spottier).scale = glm::vec3(0.2f);
        cosmos->get_component<TransformComponent>(spottier).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        // remember this is relative to exposure
        cosmos->add_component(spottier, LightSourceComponent(glm::vec3(6.0f, 3.0f, 2.0f)));
        cosmos->get_component<LightSourceComponent>(spottier).type = LightSourceType::spot;
        cosmos->get_component<LightSourceComponent>(spottier).composition.x = 0.2f;
        cosmos->get_component<LightSourceComponent>(spottier).attributes = glm::vec2{0.90f, 0.70f};

        RenderableComponent spottier_renderable;
        spottier_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
        spottier_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(spottier, spottier_renderable);

        // *************************************************************************** */
        /***************************************************************************
        ModelCache::ImportReceipt picto_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - The Legend of Zelda The Wind Waker - Picto Box\\pictobox.obj");

        Entity picto = cosmos->create_entity();
        cosmos->add_component(picto, TransformComponent(glm::vec3(-12.6f, 4.0f, 0.0f)));

        RenderableComponent picto_renderable;
        for (const std::string meshName : picto_import.meshNames)
            picto_renderable.meshData.push_Back(ModelCache::fetch_mesh(meshName));
        for (const std::string matName : picto_import.materialNames)
            picto_renderable.materials.push_back(ModelCache::fetch_material(matName));
        picto_renderable.localTransform.scale = { 0.5f, 0.5f, 0.5f };
        cosmos->add_component(picto, picto_renderable);
        // ***************************************************************************/


        /***************************************************************************
        ModelCache::ImportReceipt meteor_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\3DS - Pokemon Super Mystery Dungeon - Meteor\\Meteor\\ev01_meteor.obj");

        Entity meteor = cosmos->create_entity();
        TransformComponent meteor_transform;
        meteor_transform.origin = { 12.0f, -1.5f, -1.5f };
        meteor_transform.orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        cosmos->add_component(meteor, meteor_transform);

        RenderableComponent meteor_renderable;
        for (const std::string meshName : meteor_import.meshNames)
            meteor_renderable.meshData.push_back(ModelCache::fetch_mesh(meshName));
        for (const std::string matName : meteor_import.materialNames)
            meteor_renderable.materials.push_back(ModelCache::fetch_material(matName));
        meteor_renderable.localTransform.scale = { 0.5f, 0.5f, 0.5f };
        cosmos->add_component(meteor, meteor_renderable);
        // ***************************************************************************/

        //***************************************************************************
        Entity point = cosmos->create_entity();
        cosmos->add_component(point, TransformComponent(glm::vec3(12.0f, -1.0f, 0.0f)));
        //cosmos->get_component<TransformComponent>(point).scale = glm::vec3(0.2f);
        // remember this is relative to exposure
        cosmos->add_component(point, LightSourceComponent(glm::vec3(2.0f, 3.0f, 6.0f)));

        RenderableComponent point_renderable;
        point_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
        point_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(point, point_renderable);
        
        PhysicsComponent point_physics;
        point_physics.mass = 100.0f;
        cosmos->add_component(point, point_physics);
        
        ColliderComponent gamebox_collider{ 
            { Collider(ColliderType::sphere, CollisionType::rigid) }
        };
        gamebox_collider.colliders[0].dynamicFriction = 0.9f;
        gamebox_collider.colliders[0].restitution = 0.1f;
        cosmos->add_component(point, gamebox_collider);
        // ***************************************************************************/

        return cosmos;
    }
}