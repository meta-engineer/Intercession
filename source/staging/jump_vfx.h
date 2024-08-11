#ifndef JUMP_VFX_H
#define JUMP_VFX_H

//#include "intercession_pch.h"
#include <memory>
#include <glm/gtc/random.hpp>

#include "staging/cosmos_builder.h"
#include "behaviors/projectile_component.h"

namespace pleep
{
    // returns single entity launched from origin in direction
    inline Entity create_jump_vfx(
        std::shared_ptr<Cosmos> cosmos,
        Entity jumper,
        glm::vec3 origin
    )
    {
        assert(cosmos);

        Entity vfx = cosmos->create_entity(true, jumper);
        //Entity vfx = cosmos->create_entity();
        TransformComponent vfx_transform(origin);
        //vfx_transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
        cosmos->add_component(vfx, vfx_transform);
        
        RenderableComponent vfx_renderable;
        vfx_renderable.meshData.push_back(ModelCache::fetch_mesh(ModelCache::BasicMeshType::icosahedron));
        if (ModelCache::fetch_material("vfx_mat") == nullptr)
        {
            ModelCache::create_material("vfx_mat", std::unordered_map<TextureType, std::string>{
                {TextureType::diffuse,  "resources/blending_transparent_window.png"},
                {TextureType::specular, "resources/snow-packed12-Specular.png"},
                {TextureType::normal,   "resources/snow-packed12-normal-ogl.png"},
                //{TextureType::height,   "resources/snow-packed12-Height.png"},
                {TextureType::emissive, "resources/snow-packed12-Specular.png"}
            });
        }
        vfx_renderable.materials.push_back(ModelCache::fetch_material("vfx_mat"));
        cosmos->add_component(vfx, vfx_renderable);

        
        PhysicsComponent vfx_physics;
        vfx_physics.angularVelocity = glm::sphericalRand(6.0f);;
        vfx_physics.lockOrigin = true;
        vfx_physics.lockedOrigin = origin;
        cosmos->add_component(vfx, vfx_physics);
        // no collider

        // Add behavior to timeout and disappear
        ProjectileComponent vfx_projectile;
        vfx_projectile.maxLifetime = 1.0;
        vfx_projectile.hz = 4.0;
        vfx_projectile.scaleMin = 0.0;
        vfx_projectile.scaleMax = 2.0;
        cosmos->add_component(vfx, vfx_projectile);

        BehaviorsComponent vfx_behaviors;
        vfx_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::projectile);
        vfx_behaviors.use_fixed_update = true;
        cosmos->add_component(vfx, vfx_behaviors);

        return vfx;
    }
}

#endif // JUMP_VFX_H