#ifndef DEBUG_RENDER_PACKET_H
#define DEBUG_RENDER_PACKET_H

//#include "intercession_pch.h"
#include "ecs/ecs_types.h"
#include "physics/transform_component.h"
#include "rendering/model_manager.h"

namespace pleep
{
    // Generic renderable definition with no references to an entity
    // useful for collider rendering
    // contains a COPY of the composed transform, so a little more memory than just references
    struct DebugRenderPacket
    {
        Entity entity;
        glm::mat4 transform;
        ModelManager::BasicMeshType meshType;
    };
}

#endif // DEBUG_RENDER_PACKET_H