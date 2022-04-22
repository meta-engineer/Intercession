#ifndef SOTC_CAMERA_CONTROL_RELAY_H
#define SOTC_CAMERA_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include "controlling/i_control_relay.h"
#include "controlling/spacial_input_buffer.h"
#include "physics/transform_component.h"
#include "physics/physics_component.h"

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
        
        void engage(double deltaTime) override
        {
            while (!m_controlPacketQueue.empty())
            {
                ControlPacket data = m_controlPacketQueue.front();
                m_controlPacketQueue.pop();

                // we don't have any guarentee that component exists without specific synchro signature
                try
                {
                    TransformComponent& transform = data.owner->get_component<TransformComponent>(data.controllee);
                    //PhysicsComponent& physics = data.owner->get_component<PhysicsComponent>(data.controllee);
                    const glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
                    glm::vec3 viewDir = transform.get_heading();
                    glm::vec3 tangent = glm::normalize(glm::cross(viewDir, gimbalUp));
                    glm::vec3 targetDir = glm::vec3(0.0f, 0.0f, 1.0f);
                    glm::vec3 targetVel = glm::vec3(0.0f);
                    glm::vec3 targetVelTangent = glm::vec3(0.0f);

                    // TODO: move these constants to be stored by components?
                    const float disp   = 2.0f * (float)deltaTime;
                    const float rot    = 0.15f * (float)deltaTime;
                    const float aspect = 1.2f;
                    const float gimbalLimit = 0.1f;  // rads
                    const float springConstant = 1.0f;
                    
                    // determine position for view to target
                    glm::vec3 targetPosition(0.0f);

                    // get TransformComponent of target
                    if (data.controller.target != NULL_ENTITY)
                    {
                        try 
                        {
                            TransformComponent& targetTransform = data.owner->get_component<TransformComponent>(data.controller.target);
                            targetPosition = targetTransform.origin;
                            targetDir = targetTransform.get_heading();
                        }
                        catch(const std::range_error& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            PLEEPLOG_WARN(err.what());
                            PLEEPLOG_WARN("Control component has target which is invalid or has no TransformComponent, clearing and skipping");
                            data.controller.target = NULL_ENTITY;
                        }
                    }
                    // get PhysicsComponent of target
                    if (data.controller.target != NULL_ENTITY)
                    {
                        try 
                        {
                            PhysicsComponent& targetPhysics = data.owner->get_component<PhysicsComponent>(data.controller.target);
                            targetVel = targetPhysics.velocity;
                            if (targetVel != glm::vec3(0.0f))
                                targetVelTangent = glm::normalize(glm::cross(targetVel, gimbalUp));
                        }
                        catch(const std::range_error& err)
                        {
                            UNREFERENCED_PARAMETER(err);
                            // we can operate without velocity
                            //PLEEPLOG_WARN(err.what());
                        }
                    }
                    // target may now to reset as NULL_ENTITY
                    if (data.controller.target == NULL_ENTITY)
                    {
                        // set target to be current coordinate (before rotating)
                        targetPosition = transform.origin
                            - (-viewDir * data.controller.range)
                            - (data.controller.dynamicOffset)
                            - (data.controller.targetOffset);
                    }
                    
                    // track rotation from mouse input
                    if (m_inputCache.actions.test(SpacialActions::rotatePitchUp)
                    && !m_inputCache.actions.test(SpacialActions::rotatePitchDown))
                    {
                        transform.orientation = glm::angleAxis(rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp), tangent) * transform.orientation;
                        glm::vec3 newDirection = transform.get_heading();

                        // check if we have skipped past the singularity buffer
                        // true is positive
                        const bool oldSign = (viewDir.x * viewDir.z) >= 0;
                        const bool newSign = (newDirection.x * newDirection.z) >= 0;
                        if (oldSign == newSign && (viewDir.x / std::abs(viewDir.x)) != (newDirection.x / std::abs(newDirection.x)))
                        {
                            // rotate back around by same amount + small amount into the limit buffer
                            transform.orientation = glm::angleAxis(-rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp) + gimbalLimit/10.0f, tangent) * transform.orientation;
                            newDirection = transform.get_heading();
                        }

                        // limit pitch around gimbal singularity
                        // if heading & gimbal are normalized, dot product = cosAngle
                        // if angle < K we need to counter-rotate by quat of angle along tangent.
                        const float gimbalAngle = glm::acos(glm::dot(newDirection, gimbalUp));
                        if (gimbalAngle < gimbalLimit)
                        {
                            transform.orientation = glm::angleAxis(-(gimbalLimit - gimbalAngle), tangent) * transform.orientation;
                        }
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotatePitchDown)
                    && !m_inputCache.actions.test(SpacialActions::rotatePitchUp))
                    {
                        transform.orientation = glm::angleAxis(-rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown), tangent) * transform.orientation;
                        glm::vec3 newDirection = transform.get_heading();
                        
                        const bool oldSign = (viewDir.x * viewDir.z) >= 0;
                        const bool newSign = (newDirection.x * newDirection.z) >= 0;
                        if (oldSign == newSign && (viewDir.x / std::abs(viewDir.x)) != (newDirection.x / std::abs(newDirection.x)))
                        {
                            // rotate back around by same amount - small amount into the limit buffer
                            transform.orientation = glm::angleAxis(rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown) - gimbalLimit/10.0f, tangent) * transform.orientation;
                            newDirection = transform.get_heading();
                        }

                        // use -gimbalUp for pitch down
                        const float gimbalAngle = glm::acos(glm::dot(newDirection, -gimbalUp));
                        if (gimbalAngle < gimbalLimit)
                        {
                            transform.orientation = glm::angleAxis(gimbalLimit - gimbalAngle, tangent) * transform.orientation;
                        }
                        
                    }

                    // remember Ccw is positive
                    if (m_inputCache.actions.test(SpacialActions::rotateYawLeft)
                    && !m_inputCache.actions.test(SpacialActions::rotateYawRight))
                    {
                        transform.orientation = glm::angleAxis(rot * aspect * m_inputCache.actionVals.at(SpacialActions::rotateYawLeft), gimbalUp) * transform.orientation;
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotateYawRight)
                    && !m_inputCache.actions.test(SpacialActions::rotateYawLeft))
                    {
                        transform.orientation = glm::angleAxis(-rot * aspect * m_inputCache.actionVals.at(SpacialActions::rotateYawRight), gimbalUp) * transform.orientation;
                    }

                    // recalculate new direction vectors
                    viewDir = transform.get_heading();
                    //tangent = glm::normalize(glm::cross(viewDir, gimbalUp));

                    // adjust offset with spring model
                    // we want to place camera view to left or right of target, tangent to the direction they're moving
                    // determine left or right by whichever is closer to current
                    glm::vec3 springRestOffset = targetVelTangent * data.controller.maxDynamicOffset;
                    springRestOffset *= 
                        glm::length2(springRestOffset - data.controller.dynamicOffset) 
                        < glm::length2(-springRestOffset - data.controller.dynamicOffset) 
                        ? 1.0f : -1.0f;
                    const glm::vec3 deltaOffset = springRestOffset - data.controller.dynamicOffset;

                    data.controller.dynamicOffset += deltaOffset * springConstant * (float)deltaTime;

                    // given set rotation, set origin such that controller target is at offset distance
                    transform.origin = targetPosition
                        + (data.controller.targetOffset)
                        + (data.controller.dynamicOffset)
                        + (-viewDir * data.controller.range);
                }
                catch(const std::range_error& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    PLEEPLOG_WARN(err.what());
                    PLEEPLOG_WARN("Could not retrieve component for entity: " + std::to_string(data.controllee));
                    continue;
                }
                
            }
        }
        

    private:
        // reference to data updated in dynamo
        // this could be copied each update for safety, but this is probably technically faster
        const SpacialInputBuffer & m_inputCache;
        
    };
}

#endif // SOTC_CAMERA_CONTROL_RELAY_H