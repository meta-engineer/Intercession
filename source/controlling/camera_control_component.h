#ifndef CAMERA_CONTROL_COMPONENT_H
#define CAMERA_CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#include "controlling/i_control_component.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    // "camera based" controllers can come in many flavours
    enum CameraControlType
    {
        basic,
        sotc,
        drone
    };

    struct CameraControlComponent : public IControlComponent
    {
        // ***** camera CONTROLLER specific attributes *****
        // not to be confused with camera rendering attributes

        // track another entity(s) in view
        Entity m_target             = NULL_ENTITY;
        // local offset from target origin to track (their head for example)
        // This may have to be fetched from that entity when targeting starts...
        glm::vec3 m_targetOffset    = glm::vec3(0.0f);
        // flexable offset to allow dynamic framing of target
        // Again, parameters for framing might need to be fetched somehow...
        //glm::vec3 m_dynamicOffset   = glm::vec3(0.0f);
        //float m_maxDynamicOffset    = 0.0f;
        // Depth away from target + offsets along negative camera heading
        float m_range               = 10.0f;
        float m_minRange            = 2.0f;
        float m_maxRange            = 10.0f;

        // strength of non-linear movements
        float springConstant = 4.0f;
        // direction of euler singularity
        glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
        // restrict movement around euler singularity
        float gimbalLimit = 0.1f;  // rads

        // camera controller may also modify CameraComponent members of same entity (in packet)

        // designate desired relay type dispatched by dynamo
        CameraControlType m_type = CameraControlType::sotc;
    };
}

#endif // CAMERA_CONTROL_COMPONENT_H