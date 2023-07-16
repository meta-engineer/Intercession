#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "logging/pleep_log.h"
#include "physics/transform_component.h"

namespace pleep
{
    enum ProjectionType
    {
        perspective,
        orthographic
    };

    struct CameraComponent
    {
        // use TransformComponent.position and .orientation

        // ***** rendering members *****
        ProjectionType projectionType = ProjectionType::perspective;
        glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0f);
        float        viewNear   = 0.1f;
        float        viewFar    = 100.0f;
        unsigned int viewWidth  = 1024;
        unsigned int viewHeight = 1024;
        float        viewFov    = 45.0f;

        // ***** control members (used by behaviors) *****
        // target entity
        Entity target = NULL_ENTITY;
        // ranges/distances
        float range = 10.0f;
        // spring joint variables
        // gimbal meta-data
        // margins/limits
    };

    // Helper function for camera use
    // use camera entity's transform and camera data to build world_to_view transform
    inline glm::mat4 get_lookAt(TransformComponent& trans, CameraComponent& cam)
    {
        // recalculate direction vector each time since transform component only stores euler angles (in radians)
        glm::vec3 direction = trans.get_heading();
        //glm::vec3 rolledGimbal = glm::normalize(glm::vec3(glm::rotate(trans.orientation,  glm::vec4(cam.gimbalUp, 0.0f))));
        return glm::lookAt(trans.origin, trans.origin + direction, cam.gimbalUp);
    }

    // Helper function for camera use
    // use camera entity's camera data to get projection matrix (view_to_screen)
    inline glm::mat4 get_projection(CameraComponent& cam)
    {
        switch(cam.projectionType)
        {
            case ProjectionType::perspective:
            {
                return glm::perspective(
                    glm::radians(cam.viewFov), 
                    (float)cam.viewWidth/(float)cam.viewHeight, 
                    cam.viewNear, cam.viewFar
                );
            }
            case ProjectionType::orthographic:
            {
                // orth needs left, right, bottom, top
                // view fov 45 -> orthoRatio ~100?
                float orthoRatio = (45.0f / cam.viewFov) * 100;
                return glm::ortho(
                    -1 * (float)(cam.viewWidth)/orthoRatio, 
                    1 * (float)(cam.viewWidth)/orthoRatio, 
                    -1 * (float)(cam.viewHeight)/orthoRatio, 
                    1 * (float)(cam.viewHeight)/orthoRatio, 
                    cam.viewNear, cam.viewFar
                );
            }
        }

        
        PLEEPLOG_ERROR("Camera ProjectionType not recognized: " + std::to_string(cam.projectionType));
        return glm::mat4(1.0);
    }
}

#endif // CAMERA_COMPONENT_H