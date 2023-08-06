#ifndef TEST_PROJECTILE_H
#define TEST_PROJECTILE_H

//#include "intercession_pch.h"
#include <memory>

#include "staging/cosmos_builder.h"
#include "behaviors/projectile_component.h"

namespace pleep
{
    // returns single entity launched from origin in direction
    inline Entity create_test_projectile(
        std::shared_ptr<Cosmos> cosmos,
        Entity shooter,
        glm::vec3 origin,
        glm::vec3 direction
    )
    {
        assert(cosmos);

        Entity thrown = cosmos->create_entity(true, shooter);
        TransformComponent thrown_transform(origin);
        thrown_transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->add_component(thrown, thrown_transform);
        
        RenderableComponent thrown_renderable;
        thrown_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::cube);
        if (ModelCache::fetch_material("thrown_mat") == nullptr)
        {
            ModelCache::create_material("thrown_mat", std::unordered_map<TextureType, std::string>{
                {TextureType::diffuse, "resources/awesomeface.png"},
                {TextureType::specular, "resources/snow-packed12-Specular.png"},
                {TextureType::normal, "resources/snow-packed12-Normal-ogl.png"}
            });
        }
        thrown_renderable.materials.push_back(ModelCache::fetch_material("thrown_mat"));
        cosmos->add_component(thrown, thrown_renderable);

        
        PhysicsComponent thrown_physics;
        thrown_physics.velocity = direction;
        thrown_physics.angularVelocity = glm::vec3(1.1f, 2.1f, 0.2f);
        thrown_physics.mass = 10.0f;
        cosmos->add_component(thrown, thrown_physics);
        cosmos->add_component(thrown, BoxColliderComponent{});        
        cosmos->add_component(thrown, RigidBodyComponent{});

        // Add behavior to timeout and disappear
        ProjectileComponent thrown_projectile;
        cosmos->add_component(thrown, thrown_projectile);
        BehaviorsComponent thrown_behaviors;
        thrown_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::projectile);
        thrown_behaviors.use_fixed_update = true;
        cosmos->add_component(thrown, thrown_behaviors);

        return thrown;
    }
}

#endif // TEST_PROJECTILE_H