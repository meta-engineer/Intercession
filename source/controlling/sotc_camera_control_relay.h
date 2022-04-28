#ifndef SOTC_CAMERA_CONTROL_RELAY_H
#define SOTC_CAMERA_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include <queue>
#include "controlling/i_control_relay.h"
#include "controlling/spacial_input_buffer.h"
#include "controlling/camera_control_packet.h"
#include "physics/physics_component.h"
#include "core/cosmos.h"

namespace pleep
{
    // inherit control packet queue and submitting,
    // controller should place entity looking at target
    // where should the behaviour specific attributes be stored?
    // should every type of controller be its own component?
    // should all controller attributes be in a single control component?
    class SotcCameraControlRelay : public IControlRelay
    {
    public:
        SotcCameraControlRelay(const SpacialInputBuffer& actions)
            : m_inputCache(actions)
        {}
        
        void submit(CameraControlPacket data)
        {
            m_controlPacketQueue.push(data);
        }
        
        void engage(double deltaTime) override
        {
            while (!m_controlPacketQueue.empty())
            {
                CameraControlPacket data = m_controlPacketQueue.front();
                m_controlPacketQueue.pop();

                glm::vec3 viewDir = data.transform.get_heading();
                // determine coordinate where view is currently focused
                glm::vec3 viewFocus = data.transform.origin
                        + (viewDir * data.controller.m_range);
                
                glm::vec3 tangent = glm::normalize(glm::cross(viewDir, data.controller.gimbalUp));
                glm::vec3 targetDir = glm::vec3(0.0f, 0.0f, 1.0f);
                glm::vec3 targetVel = glm::vec3(0.0f);
                glm::vec3 targetVelTangent = glm::vec3(0.0f);

                // TODO: move these constants to be stored by component?
                const float rot    = 0.15f * (float)deltaTime;
                const float aspect = 1.2f;
                
                // determine position for view to target
                // (if there is no target entity, default to viewFocus)
                glm::vec3 targetPosition = viewFocus;

                // get TransformComponent of target
                if (data.controller.m_target != NULL_ENTITY)
                {
                    try 
                    {
                        TransformComponent& targetTransform = data.owner->get_component<TransformComponent>(data.controller.m_target);
                        targetPosition = targetTransform.origin;
                        targetDir = targetTransform.get_heading();
                    }
                    catch(const std::range_error& err)
                    {
                        UNREFERENCED_PARAMETER(err);
                        PLEEPLOG_WARN(err.what());
                        PLEEPLOG_WARN("Control component has target which is invalid or has no TransformComponent, clearing and skipping");
                        data.controller.m_target = NULL_ENTITY;
                    }
                }
                // get PhysicsComponent of target
                if (data.controller.m_target != NULL_ENTITY)
                {
                    try 
                    {
                        PhysicsComponent& targetPhysics = data.owner->get_component<PhysicsComponent>(data.controller.m_target);
                        targetVel = targetPhysics.velocity;
                        if (targetVel != glm::vec3(0.0f))
                            targetVelTangent = glm::normalize(glm::cross(targetVel, data.controller.gimbalUp));
                    }
                    catch(const std::range_error& err)
                    {
                        UNREFERENCED_PARAMETER(err);
                        // we can operate without velocity
                        //PLEEPLOG_WARN(err.what());
                    }
                }
                
                // track rotation from mouse input
                if (m_inputCache.actions.test(SpacialActions::rotatePitchUp)
                && !m_inputCache.actions.test(SpacialActions::rotatePitchDown))
                {
                    data.transform.orientation = glm::angleAxis(rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp), tangent) * data.transform.orientation;
                    glm::vec3 newDirection = data.transform.get_heading();

                    // check if we have skipped past the singularity buffer
                    // true is positive
                    const bool oldSign = (viewDir.x * viewDir.z) >= 0;
                    const bool newSign = (newDirection.x * newDirection.z) >= 0;
                    if (oldSign == newSign && (viewDir.x / std::abs(viewDir.x)) != (newDirection.x / std::abs(newDirection.x)))
                    {
                        // rotate back around by same amount + small amount into the limit buffer
                        data.transform.orientation = glm::angleAxis(-rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp) + data.controller.gimbalLimit/10.0f, tangent) * data.transform.orientation;
                        newDirection = data.transform.get_heading();
                    }

                    // limit pitch around gimbal singularity
                    // if heading & gimbal are normalized, dot product = cosAngle
                    // if angle < K we need to counter-rotate by quat of angle along tangent.
                    const float gimbalAngle = glm::acos(glm::dot(newDirection, data.controller.gimbalUp));
                    if (gimbalAngle < data.controller.gimbalLimit)
                    {
                        data.transform.orientation = glm::angleAxis(-(data.controller.gimbalLimit - gimbalAngle), tangent) * data.transform.orientation;
                    }
                }
                if (m_inputCache.actions.test(SpacialActions::rotatePitchDown)
                && !m_inputCache.actions.test(SpacialActions::rotatePitchUp))
                {
                    data.transform.orientation = glm::angleAxis(-rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown), tangent) * data.transform.orientation;
                    glm::vec3 newDirection = data.transform.get_heading();
                    
                    const bool oldSign = (viewDir.x * viewDir.z) >= 0;
                    const bool newSign = (newDirection.x * newDirection.z) >= 0;
                    if (oldSign == newSign && (viewDir.x / std::abs(viewDir.x)) != (newDirection.x / std::abs(newDirection.x)))
                    {
                        // rotate back around by same amount - small amount into the limit buffer
                        data.transform.orientation = glm::angleAxis(rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown) - data.controller.gimbalLimit/10.0f, tangent) * data.transform.orientation;
                        newDirection = data.transform.get_heading();
                    }

                    // use -gimbalUp for pitch down
                    const float gimbalAngle = glm::acos(glm::dot(newDirection, -(data.controller.gimbalUp)));
                    if (gimbalAngle < data.controller.gimbalLimit)
                    {
                        data.transform.orientation = glm::angleAxis(data.controller.gimbalLimit - gimbalAngle, tangent) * data.transform.orientation;
                    }
                    
                }

                // remember Ccw is positive
                if (m_inputCache.actions.test(SpacialActions::rotateYawLeft)
                && !m_inputCache.actions.test(SpacialActions::rotateYawRight))
                {
                    data.transform.orientation = glm::angleAxis(rot * aspect * m_inputCache.actionVals.at(SpacialActions::rotateYawLeft), data.controller.gimbalUp) * data.transform.orientation;
                }
                if (m_inputCache.actions.test(SpacialActions::rotateYawRight)
                && !m_inputCache.actions.test(SpacialActions::rotateYawLeft))
                {
                    data.transform.orientation = glm::angleAxis(-rot * aspect * m_inputCache.actionVals.at(SpacialActions::rotateYawRight), data.controller.gimbalUp) * data.transform.orientation;
                }
                
                // adjust viewFocus with spring model
                const glm::vec3 offset = (targetPosition - viewFocus) * (float)deltaTime;
                viewFocus += data.controller.springConstant * offset;
                
                // recalculate new direction vectors
                viewDir = data.transform.get_heading();
                //tangent = glm::normalize(glm::cross(viewDir, gimbalUp));

                // adjust camera range with spring model
                float rangeFactor = std::abs(glm::dot(data.controller.gimbalUp, viewDir));
                // non-linear factor
                rangeFactor *= rangeFactor;
                // determine target position based on rangeFactor
                const float rangeTarget = rangeFactor * (data.controller.m_maxRange - data.controller.m_minRange) + data.controller.m_minRange;
                const float rangeOffset = (rangeTarget - data.controller.m_range) * (float)deltaTime;
                data.controller.m_range += data.controller.springConstant * rangeOffset;

                // after camera is oriented and viewFocus is adjusted restore origin to viewFocus
                data.transform.origin = viewFocus - (viewDir * data.controller.m_range);
            }
        }
        

    private:
        // reference to data updated in dynamo
        // this could be copied each update for safety, but this is probably technically faster
        const SpacialInputBuffer & m_inputCache;
        
        // store all entities receiving controls this frame and defer processing after all are submitted
        // this might not be necessary, are there any control schemes which are non-greedy?
        std::queue<CameraControlPacket> m_controlPacketQueue;
    };
}

#endif // SOTC_CAMERA_CONTROL_RELAY_H