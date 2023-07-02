#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

//#include "intercession_pch.h"

#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace pleep
{
    // Provide baseline 3D orientation for all "spacial" entities
    struct TransformComponent
    {
        // should I use my own vec3's?
        glm::vec3 origin      = glm::vec3(0.0f);
        glm::quat orientation = glm::quat(glm::vec3(0.0f));
        glm::vec3 scale       = glm::vec3(1.0f);

        // "lock" flag for space-time modification of this entity
        bool superposition    = false;

        // default constructors are needed for ComponentArray
		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& origin)
            : origin(origin)
        {}

        glm::mat4 get_model_transform() const
        {
            // faster to store and maintain as matrix transforms,
            // or calculate matrices on each call?
            // glm seems to invert pitch?
            const glm::mat4 model_to_world = glm::translate(glm::mat4(1.0f), origin)
                                        * glm::toMat4(orientation)
                                        * glm::scale(glm::mat4(1.0f), scale);
            return model_to_world;
        }

        glm::vec3 get_heading() const
        {
            // un-rotated heading is (0,0,1) (looking along z axis)
            return glm::normalize(glm::vec3(glm::rotate(orientation,  glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))));
        }
    };

    inline bool operator==(const TransformComponent& lhs, const TransformComponent& rhs)
    {
        return lhs.origin == rhs.origin
            && lhs.orientation == rhs.orientation
            && lhs.scale == rhs.scale;
    }
}

#endif // TRANSFORM_COMPONENT_H