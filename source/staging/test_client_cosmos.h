#ifndef TEST_CLIENT_COSMOS_H
#define TEST_CLIENT_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "staging/dynamo_cluster.h"
#include "staging/cosmos_builder.h"
#include "logging/pleep_log.h"
#include "rendering/model_cache.h"
#include "scripting/script_library.h"

#include "staging/test_hard_config.h"

namespace pleep
{
    // build only client side entities:
    // - camera
    std::shared_ptr<Cosmos> build_test_client_cosmos(
        EventBroker* eventBroker, 
        DynamoCluster& dynamoCluster
    )
    {
        std::shared_ptr<Cosmos> cosmos = build_test_hard_config(eventBroker, dynamoCluster);

        // ***************************************************************************
        // Create interface entity for this client
        // It is a incorporeal "singleton" which collects all the client side interactions and manipulates our assigned entity



        // ***************************************************************************


        // ***************************************************************************
        // Scene needs to create an entity with camera component
        Entity mainCamera = cosmos->create_entity();
        cosmos->add_component(mainCamera, TransformComponent(glm::vec3(4.0f, 4.0f, 2.5f)));
        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        cosmos->add_component(mainCamera, mainCamera_physics);
        cosmos->get_component<TransformComponent>(mainCamera).orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, -0.7f));
        cosmos->add_component(mainCamera, CameraComponent());

        cosmos->add_component(mainCamera, SpacialInputComponent());
        ScriptComponent camera_scripts;
        camera_scripts.drivetrain = ScriptLibrary::fetch_script(ScriptLibrary::ScriptType::fly_control);
        camera_scripts.use_fixed_update = true;
        cosmos->add_component(mainCamera, camera_scripts);

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

        return cosmos;
    }
}

#endif // TEST_CLIENT_COSMOS_H