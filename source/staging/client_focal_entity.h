#ifndef CLIENT_FOCAL_ENTITY_H
#define CLIENT_FOCAL_ENTITY_H

//#include "intercession_pch.h"
#include <memory>

#include "staging/cosmos_builder.h"

namespace pleep
{
    // returns single entity to be created each time a client joins
    // client is signalled to use this entity as its main entity;
    inline Entity create_client_focal_entity(
        std::shared_ptr<Cosmos> cosmos,
        std::shared_ptr<EventBroker> eventBroker
    )
    {
        // we have to implicitly know what components are registered
        assert(cosmos);
        UNREFERENCED_PARAMETER(eventBroker);

        // ***************************************************************************
        ModelCache::ImportReceipt olimar_import = ModelCache::import("C:\\Users\\Stephen\\Repos\\Intercession_design\\Wii - Super Smash Bros Brawl - Olimar & Pikmin\\Olimar.obj");

        //ModelManager::debug_receipt(olimar_import);

        Entity pc = cosmos->create_entity();
        TransformComponent pc_transform(glm::vec3(0.0f, 3.0f, 0.0f));
        cosmos->add_component(pc, pc_transform);

        RenderableComponent pc_renderable;
        //pc_renderable.meshData = ModelCache::fetch_supermesh(ModelCache::BasicSupermeshType::cube);
        pc_renderable.meshData = ModelCache::fetch_supermesh(olimar_import.supermeshName);
        for (auto matname : olimar_import.supermeshMaterialNames)
            pc_renderable.materials.push_back(ModelCache::fetch_material(matname));
        pc_renderable.localTransform.origin.y = -0.5f;
        pc_renderable.localTransform.scale = { 0.15f, 0.15f, 0.15f };
        cosmos->add_component(pc, pc_renderable);

        PhysicsComponent pc_physics;
        //pc_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        //pc_physics.angularVelocity = glm::vec3(0.1f, 0.1f, 0.2f);
        //pc_physics.lockOrientation = true;
        pc_physics.lockedOrientation = pc_transform.orientation;
        pc_physics.mass = 40.0f;
        cosmos->add_component(pc, pc_physics);
        ColliderComponent pc_collider{ 
            { Collider(ColliderType::box, CollisionType::rigid),
              Collider(ColliderType::ray, CollisionType::spring) }
        };

        // ray "legs" are the second collider
        Collider& pc_ray = pc_collider.colliders[1];

        pc_ray.localTransform.orientation = glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));   // this should be along biped "support" axis
        pc_ray.localTransform.scale = glm::vec3(1.0f, 1.0f, 1.5f);
        pc_ray.collisionType = CollisionType::spring;
        pc_ray.inheritOrientation = false;
        pc_ray.useBehaviorsResponse = true;     // to set grounded state for on_fixed behaviour
        pc_ray.influenceOrientation = false;
        pc_ray.stiffness = 2500.0f;
        pc_ray.damping = 400.0f;
        pc_ray.restLength = 0.3f;
        pc_ray.staticFriction = 0.0f;
        pc_ray.dynamicFriction = 0.0f;
        cosmos->add_component(pc, pc_collider);

        // This entity will be set as main for its respective client, so it will only 
        // receive input on that client, and we will only allow input from that client
        cosmos->add_component(pc, SpacialInputComponent{});
        cosmos->add_component(pc, BipedComponent{});
        BehaviorsComponent pc_behaviors;
        pc_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::biped_control);
        pc_behaviors.use_fixed_update = true;
        cosmos->add_component(pc, pc_behaviors);

        // ***************************************************************************

        return pc;
    }
}

#endif // CLIENT_FOCAL_ENTITY_H