#ifndef BUILD_PC_H
#define BUILD_PC_H

//#include "intercession_pch.h"
#include <memory>

#include "staging/cosmos_builder.h"

namespace pleep
{
    inline Entity build_pc(std::shared_ptr<Cosmos> cosmos)
    {
        // we have to implicitly know what components are registered
        assert(cosmos);

        // ***************************************************************************
        Entity pc = cosmos->create_entity();
        TransformComponent pc_transform(glm::vec3(3.0f, 3.0f, -2.5f));
        cosmos->add_component(pc, pc_transform);

        RenderableComponent pc_renderable;
        pc_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::cube);
        ModelCache::create_material("pc_mat", std::unordered_map<TextureType, std::string>{
            {TextureType::diffuse, "resources/container.jpg"},
            {TextureType::specular, "resources/snow-packed12-Specular.jpg"},
            {TextureType::normal, "resources/snow-packed12-Normal-ogl.png"}
        });
        pc_renderable.materials.push_back(ModelCache::fetch_material("pc_mat"));
        cosmos->add_component(pc, pc_renderable);

        PhysicsComponent pc_physics;
        //pc_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        pc_physics.angularVelocity = glm::vec3(0.1f, 0.1f, 0.2f);
        pc_physics.mass = 100.0f;
        cosmos->add_component(pc, pc_physics);
        cosmos->add_component(pc, BoxColliderComponent{});
        cosmos->add_component(pc, RigidBodyComponent{});
        // ***************************************************************************

        return pc;
    }
}

#endif // BUILD_PC_H