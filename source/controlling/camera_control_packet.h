#ifndef CAMERA_CONTROL_PACKET_H
#define CAMERA_CONTROL_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "controlling/camera_control_component.h"
#include "rendering/camera_component.h"

namespace pleep
{
    // Forward declare: Relay needs dynamic access to ecs for other components
    class Cosmos;

    struct CameraControlPacket
    {
        CameraControlComponent& controller;
        TransformComponent& transform;
        // control relay needs access to potentially read attributes of target entities
        // (we can also use it to check/fetch the camera (render) component of this entity)
        Entity controllee;
        Cosmos* owner;
    };
}

#endif // CAMERA_CONTROL_PACKET_H