#ifndef CLIENT_LOCAL_ENTITIES_H
#define CLIENT_LOCAL_ENTITIES_H

//#include "intercession_pch.h"
#include <memory>

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
        // TODO: camera is reset every time jump and will not retain the same view...
        //  1. add camera data into server transfer... but server doesn't really care about camera
        //  2. find some place to store client side data between jumps, can save main camera before jump, and restore it here (instead of creating new one?)

        // ***************************************************************************
        // Scene needs to create an entity with camera component
        Entity mainCamera = cosmos->create_entity(false);
        cosmos->add_component(mainCamera, TransformComponent{});
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
        camera_ray.localTransform.scale = glm::vec3(1.0f, 1.0f, 40.0f);
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
}

#endif // CLIENT_LOCAL_ENTITIES_H