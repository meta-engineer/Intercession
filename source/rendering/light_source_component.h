#ifndef LIGHT_SOURCE_COMPONENT_H
#define LIGHT_SOURCE_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>

namespace pleep
{
    enum LightSourceType
    {
        point,
        spot,
        ray
    };

    struct LightSourceComponent
    {
        // rendering configuration for shaders
        LightSourceType type = LightSourceType::point;

        // rgb "wavelength"
        glm::vec3 color = glm::vec3(1.0f);
        // ambient, diffuse, specular strengths (for non-pbr)
        glm::vec3 composition = glm::vec3(0.01f, 1.0f, 1.0f);
        // constant, linear, and quadtratic falloff
        glm::vec3 attenuation = glm::vec3(1.0f, 0.14f, 0.07f);
        
        // type specific parameters (spotlight's innerCos/outerCos)
        glm::vec2 attributes;

        
		LightSourceComponent() = default;
		LightSourceComponent(const LightSourceComponent&) = default;
        LightSourceComponent(glm::vec3 color)
            : color(color)
        {}
    };
}

#endif // LIGHT_SOURCE_COMPONENT_H