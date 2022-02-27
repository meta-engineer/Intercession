#ifndef CAMERA_PACKET_H
#define CAMERA_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "rendering/camera_component.h"

namespace pleep
{
    // collection of ECS components for the render pipeline
    // we'll store component references for efficiency
    // packets should be stort lived and never outlive their parent entity
    // if there comes a day when physics and rendering are async, these may want to be copies
    //   or smart pointers so that they can live beyond entity for the frame duration
    struct CameraPacket
    {
        TransformComponent& transform;
        CameraComponent& camera;
    };
}

#endif // CAMERA_PACKET_H