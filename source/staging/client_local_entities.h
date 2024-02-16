#ifndef CLIENT_LOCAL_ENTITIES_H
#define CLIENT_LOCAL_ENTITIES_H

//#include "intercession_pch.h"
#include <memory>
#include <queue>

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"
#include "staging/cosmos_builder.h"
#include "logging/pleep_log.h"
#include "rendering/model_cache.h"
#include "behaviors/behaviors_library.h"

#include "staging/hard_config_cosmos.h"

namespace pleep
{
    // create only client side entities in cosmos:
    // - camera
    inline void create_client_local_entities(
        std::shared_ptr<Cosmos> cosmos,
        std::shared_ptr<EventBroker> eventBroker
    )
    {
        // ***************************************************************************
        // Scene needs to create an entity with camera component
        Entity mainCamera = cosmos->create_entity(false);

        TransformComponent mainCamera_transform{};
        mainCamera_transform.orientation = glm::angleAxis(glm::pi<float>(), glm::normalize(glm::vec3(0.0f,1.0f,-0.3f)));
        cosmos->add_component(mainCamera, mainCamera_transform);

        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        cosmos->add_component(mainCamera, mainCamera_physics);

        CameraComponent mainCamera_camera;
        // set camera target to be the current focal entity
        mainCamera_camera.target = cosmos->get_focal_entity();
        cosmos->add_component(mainCamera, mainCamera_camera);

        cosmos->add_component(mainCamera, SpacialInputComponent{});
        BehaviorsComponent camera_behaviors;
        camera_behaviors.drivetrain = BehaviorsLibrary::fetch_behaviors(BehaviorsLibrary::BehaviorsType::osrs_camera);
        camera_behaviors.use_fixed_update = true;
        cosmos->add_component(mainCamera, camera_behaviors);

        // ray collider for mouse click
        RayColliderComponent camera_ray;
        camera_ray.responseType = CollisionResponseType::noop;
        camera_ray.useBehaviorsResponse = true;
        camera_ray.localTransform.scale = glm::vec3(1.0f, 1.0f, 100.0f);
        cosmos->add_component(mainCamera, camera_ray);

        // empty renderable to render collider
        cosmos->add_component(mainCamera, RenderableComponent{});

        // then it needs to be assigned somewhere in render pipeline (view camera, shadow camera, etc)
        // assuming there is only ever 1 main camera we can notify over event broker
        // dynamos don't have access to cosmos, so they can't lookup entity
        // synchro can maintain camera and pass its data each frame
        EventMessage cameraEvent(events::rendering::SET_MAIN_CAMERA);
        events::rendering::SET_MAIN_CAMERA_params cameraParams {
            mainCamera
        };
        cameraEvent << cameraParams;
        eventBroker->send_event(cameraEvent);
        // ***************************************************************************
    }

    inline void cache_client_local_entities(
        std::queue<EventMessage>& jumpCache,
        std::shared_ptr<Cosmos> cosmos,
        std::shared_ptr<EventBroker> eventBroker
    )
    {
        for (std::pair<const Entity, Signature> e : cosmos->get_signatures_ref())
        {
            if (derive_timeslice_id(e.first) == NULL_TIMESLICEID) // clients should have NULL ID
            {
                jumpCache.push(EventMessage(events::cosmos::ENTITY_UPDATE));
                cosmos->serialize_entity_components(e.first, e.second, jumpCache.back());
                
                // entity value is not transferable so flag as NULL_ENTITY
                events::cosmos::ENTITY_UPDATE_params entityInfo{
                    NULL_ENTITY, e.second, ComponentCategory::all
                };
                jumpCache.back() << entityInfo;
            }
        }
    }

    inline void uncache_client_local_entities(
        std::queue<EventMessage>& jumpCache,
        std::shared_ptr<Cosmos> cosmos,
        std::shared_ptr<EventBroker> eventBroker
    )
    {
        while (!jumpCache.empty())
        {
            EventMessage entityMsg = jumpCache.front();
            jumpCache.pop();
            events::cosmos::ENTITY_UPDATE_params entityInfo;
            entityMsg >> entityInfo;
            
            // create entity
            const Entity reload = cosmos->create_entity(false);
            cosmos->deserialize_entity_components(reload, entityInfo.sign, entityMsg, entityInfo.category);


            // special cases:

            // camera (assume there will only ever be one???)
            if (cosmos->has_component<CameraComponent>(reload))
            {
                EventMessage cameraEvent(events::rendering::SET_MAIN_CAMERA);
                events::rendering::SET_MAIN_CAMERA_params cameraParams {
                    reload
                };
                cameraEvent << cameraParams;
                eventBroker->send_event(cameraEvent);
            }
        }
    }
}

#endif // CLIENT_LOCAL_ENTITIES_H