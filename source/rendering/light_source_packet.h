#ifndef LIGHT_PACKET_H
#define LIGHT_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "rendering/light_source_component.h"

namespace pleep
{
    // collection of ECS components for the render pipeline
    // we'll store component references for efficiency
    // packets should be stort lived and never outlive their parent entity
    // if there comes a day when physics and rendering are async, these may want to be copies
    //   or smart pointers so that they can live beyond entity for the frame duration
    struct LightSourcePacket
    {
        TransformComponent& transform;
        LightSourceComponent& light;
    };
}

#endif // LIGHT_PACKET_H