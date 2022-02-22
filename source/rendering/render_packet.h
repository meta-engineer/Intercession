#ifndef RENDER_PACKET_H
#define RENDER_PACKET_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"
#include "rendering/mesh.h"

namespace pleep
{
    // collection of ECS components for the render pipeline
    // we'll store component references for efficiency
    // packets should be stort lived and never outlive their parent entity
    // if there comes a day when physics and rendering are async, these may want to be copies
    //   or smart pointers so that they can live beyond entity for the frame duration
    struct RenderPacket
    {
        TransformComponent& transform;
        Mesh& mesh;
        // instancing info?
        // model-wide temporary options? pallete swap? outlines?
    };
}

#endif // RENDER_PACKET_H