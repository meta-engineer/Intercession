#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

//#include "intercession_pch.h"

#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

namespace pleep
{
    // Provide baseline 3D origin for all in-cosmos entities
    // more advanced properties (rotation, velocities, bounding boxes)
    //   will be in a physics component
    struct TransformComponent
    {
        // should I use my own vec3's?
        glm::vec3 origin;
    };
}

#endif // TRANSFORM_COMPONENT_H