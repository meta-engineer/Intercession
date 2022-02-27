#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

//#include "intercession_pch.h"

#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace pleep
{
    // Provide baseline 3D origin for all in-cosmos entities
    // more advanced properties (velocities, bounding boxes)
    //   will be in a physics component
    struct TransformComponent
    {
        // should I use my own vec3's?
        glm::vec3 origin   = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f, glm::radians(-90.0f), 0.0f);   // euler angles: pitch, yaw, roll (radians)
        glm::vec3 scale    = glm::vec3(1.0f);

        // default constructors are needed for ComponentArray
		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& origin)
            : origin(origin)
        {}

        glm::mat4 get_model_transform()
        {
            // faster to store and maintain as matrix transforms,
            // or calculate matrices on each call?
            glm::mat4 model_to_world = glm::translate(glm::mat4(1.0f), origin)
                                    * glm::toMat4(glm::quat(rotation))
                                    * glm::scale(glm::mat4(1.0f), scale);
            return model_to_world;
        }
    };
}

#endif // TRANSFORM_COMPONENT_H