#ifndef TEST_PROJECTILE_H
#define TEST_PROJECTILE_H

//#include "intercession_pch.h"
#include <memory>
#include <glm/gtc/random.hpp>

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

        UNREFERENCED_PARAMETER(shooter);
        Entity thrown = cosmos->create_entity(true, shooter);
        //Entity thrown = cosmos->create_entity();
        TransformComponent thrown_transform(origin);
        thrown_transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);
        cosmos->add_component(thrown, thrown_transform);
        
        RenderableComponent thrown_renderable;
        thrown_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::cube));
        if (ModelCache::fetch_material("thrown_mat") == nullptr)
        {
            ModelCache::create_material("thrown_mat", std::unordered_map<TextureType, std::string>{
                {TextureType::diffuse, "resources/checker.png"},
                {TextureType::specular, "resources/checker.png"},
                {TextureType::normal, "resources/flat_normal.jpg"}
            });
        }
        thrown_renderable.materials.push_back(ModelCache::fetch_material("thrown_mat"));
        cosmos->add_component(thrown, thrown_renderable);

        
        PhysicsComponent thrown_physics;
        thrown_physics.velocity = direction;
        thrown_physics.angularVelocity = glm::sphericalRand(2.0f);
        thrown_physics.mass = 100.0f;
        cosmos->add_component(thrown, thrown_physics);
        cosmos->add_component(thrown, ColliderComponent{
            { Collider(ColliderType::box, CollisionType::rigid) }
        });

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