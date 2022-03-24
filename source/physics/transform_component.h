#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

//#include "intercession_pch.h"

#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace pleep
{
    // Provide baseline 3D orientation for all "spacial" entities
    struct TransformComponent
    {
        // should I use my own vec3's?
        glm::vec3 origin   = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);   // euler angles: pitch, yaw, roll (radians)
        glm::vec3 scale    = glm::vec3(1.0f);

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
            glm::mat4 model_to_world = glm::translate(glm::mat4(1.0f), origin)
                                    * glm::eulerAngleYXZ(rotation.y, -rotation.x, rotation.z)
                                    * glm::scale(glm::mat4(1.0f), scale);
            return model_to_world;
        }

        glm::vec3 get_heading() const
        {
            // all 0 rotations -> (0,0,1) (looking along z axis)
            glm::vec3 direction(
                sin(rotation.y) * cos(rotation.x),
                sin(rotation.x),
                cos(rotation.y) * cos(rotation.x)
            );
            return glm::normalize(direction);
        }
    };
}

#endif // TRANSFORM_COMPONENT_H