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
        ModelCache::ImportReceipt gamecube_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\GameCube - Super Smash Bros Melee - GameCube\\Gamecube\\gamecube.obj");

        Entity gamebox = cosmos->create_entity();

        TransformComponent gamebox_transform(glm::vec3(-1.0f, 0.0f, -10.0f));
        gamebox_transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->add_component(gamebox, gamebox_transform);

        RenderableComponent gamebox_renderable;
        gamebox_renderable.meshData = ModelCache::fetch_supermesh(gamecube_import.supermeshName);
        // load materials in order of submeshes
        for (const std::string matName : gamecube_import.supermeshMaterialNames)
        {
            gamebox_renderable.materials.push_back(ModelCache::fetch_material(matName));
        }
        cosmos->add_component(gamebox, gamebox_renderable);

        PhysicsComponent gamebox_physics;
        gamebox_physics.mass = 200.0f;
        cosmos->add_component(gamebox, gamebox_physics);
        
        RigidBodyComponent gamebox_rigidbody{};
        gamebox_rigidbody.dynamicFriction = 0.8f;
        gamebox_rigidbody.restitution = 0.2f;
        cosmos->add_component(gamebox, gamebox_rigidbody);
        
        BoxColliderComponent gamebox_collider;
        gamebox_collider.localTransform.scale = glm::vec3(10.0f, 7.4f, 10.0f);
        cosmos->add_component(gamebox, gamebox_collider);
        // ***************************************************************************



        // ***************************************************************************
        ModelCache::create_material("floor_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/moon_diffuse.png"},
            {TextureType::specular, "resources/moon_diffuse.png"},
            {TextureType::normal, "resources/moon_normal.png"},
        });

        Entity floor = cosmos->create_entity(false);
        cosmos->add_component(floor, TransformComponent(glm::vec3(0.0f, -2.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(floor).scale = glm::vec3(50.0f, 50.0f, 0.05f);
        
        RenderableComponent floor_renderable;
        floor_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::quad);
        floor_renderable.materials.push_back(ModelCache::fetch_material("floor_mat"));
        cosmos->add_component(floor, floor_renderable);
        
        PhysicsComponent floor_physics;
        // TODO: in general generate mass from a known density
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
        ModelCache::create_material("lightbulb_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse,  "resources/blending_transparent_window.png"},
            {TextureType::specular, "resources/snow-packed12-Specular.png"},
            {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
            //{TextureType::height,   "resources/snow-packed12-Height.png"},
            {TextureType::emissive, "resources/snow-packed12-Specular.png"}
        });

        Entity spot = cosmos->create_entity();
        cosmos->add_component(spot, TransformComponent(glm::vec3(0.0f, 4.0f, 0.0f)));
        cosmos->get_component<TransformComponent>(spot).scale = glm::vec3(0.2f);
        cosmos->get_component<TransformComponent>(spot).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        // remember this is relative to exposure
        cosmos->add_component(spot, LightSourceComponent(glm::vec3(2.0f, 3.0f, 6.0f)));
        cosmos->get_component<LightSourceComponent>(spot).type = LightSourceType::spot;
        cosmos->get_component<LightSourceComponent>(spot).attributes = glm::vec2{0.90f, 0.70f};

        RenderableComponent spot_renderable;
        spot_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::icosahedron);
        spot_renderable.materials.push_back(ModelCache::fetch_material("lightbulb_mat"));
        cosmos->add_component(spot, spot_renderable);
        // ***************************************************************************



        return cosmos;
    }
}