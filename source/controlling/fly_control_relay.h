#ifndef FLY_CONTROL_RELAY_H
#define FLY_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include "controlling/i_control_relay.h"
#include "controlling/spacial_input_buffer.h"
#include "physics/transform_component.h"

namespace pleep
{
    class FlyControlRelay : public IControlRelay
    {
    public:
        // initialize safe references to my caller(dynamo)'s input buffers
        FlyControlRelay(const SpacialInputBuffer& actions)
            : m_inputCache(actions)
        {}

        void engage(double deltaTime) override
        {
            while (!m_controlPacketQueue.empty())
            {
                ControlPacket data = m_controlPacketQueue.front();
                m_controlPacketQueue.pop();

                // we don't have any guarentee that component exists without synchro signature
                try
                {
                    TransformComponent& transform = data.owner->get_component<TransformComponent>(data.controllee);

                    // generate direction vector from euler angles
                    glm::vec3 direction = transform.get_heading();
                    // units/time * time (seconds)
                    const float disp   = 2.0f * (float)deltaTime;
                    const float rot    = 0.15f * (float)deltaTime;
                    const float aspect = 1.2f;
                    const float gimbalLimit = 0.1f;  // rads
                    glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
                    glm::vec3 tangent = glm::normalize(glm::cross(direction, gimbalUp));
                    
                    // match actions to component changes (use actionVal at the same index)
                    // data of input should be standardized and coordinated with dynamo
                    if (m_inputCache.actions.test(SpacialActions::moveForward)
                    && !m_inputCache.actions.test(SpacialActions::moveBackward))
                    {
                        transform.origin += direction * disp * m_inputCache.actionVals.at(SpacialActions::moveForward);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveBackward)
                        && !m_inputCache.actions.test(SpacialActions::moveForward))
                    {
                        transform.origin -= direction * disp * m_inputCache.actionVals.at(SpacialActions::moveBackward);
                    }

                    if (m_inputCache.actions.test(SpacialActions::moveLeft)
                    && !m_inputCache.actions.test(SpacialActions::moveRight))
                    {
                        transform.origin += tangent * disp * m_inputCache.actionVals.at(SpacialActions::moveLeft);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveRight)
                    && !m_inputCache.actions.test(SpacialActions::moveLeft))
                    {
                        transform.origin -= tangent * disp * m_inputCache.actionVals.at(SpacialActions::moveRight);
                    }

                    if (m_inputCache.actions.test(SpacialActions::moveUp)
                    && !m_inputCache.actions.test(SpacialActions::moveDown))
                    {
                        // always go gimbalUp regardless of rotation
                        transform.origin += gimbalUp * disp * m_inputCache.actionVals.at(SpacialActions::moveUp);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveDown)
                    && !m_inputCache.actions.test(SpacialActions::moveUp))
                    {
                        // always go gimbalUp regardless of rotation
                        transform.origin -= gimbalUp * disp * m_inputCache.actionVals.at(SpacialActions::moveDown);
                    }

                    
                    if (m_inputCache.actions.test(SpacialActions::rotatePitchUp)
                    && !m_inputCache.actions.test(SpacialActions::rotatePitchDown))
                    {
                        transform.orientation = glm::angleAxis(rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp), tangent) * transform.orientation;
                        glm::vec3 newDirection = transform.get_heading();

                        // check if we have skipped past the singularity buffer
                        // true is positive
                        const bool oldSign = (direction.x * direction.z) >= 0;
                        const bool newSign = (newDirection.x * newDirection.z) >= 0;
                        if (oldSign == newSign && (direction.x / std::abs(direction.x)) != (newDirection.x / std::abs(newDirection.x)))
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
                        
                        const bool oldSign = (direction.x * direction.z) >= 0;
                        const bool newSign = (newDirection.x * newDirection.z) >= 0;
                        if (oldSign == newSign && (direction.x / std::abs(direction.x)) != (newDirection.x / std::abs(newDirection.x)))
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
/*
                    if (m_inputCache.actions.test(SpacialActions::rotateRollCw)
                    && !m_inputCache.actions.test(SpacialActions::rotateRollCcw))
                    {
                        transform.rotation.z += rot * m_inputCache.actionVals.at(SpacialActions::rotateRollCw);
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotateRollCcw)
                    && !m_inputCache.actions.test(SpacialActions::rotateRollCw))
                    {
                        transform.rotation.z -= rot * m_inputCache.actionVals.at(SpacialActions::rotateRollCcw);
                    }
                    transform.rotation.z = glm::max(transform.rotation.z, glm::radians(-5.0f));
                    transform.rotation.z = glm::min(transform.rotation.z, glm::radians( 5.0f));
*/
                }
                catch (const std::range_error& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    PLEEPLOG_WARN("Could not retrieve TransformComponent for entity: " + std::to_string(data.controllee));
                    continue;
                }
            }
        }

    private:
        // I don't think ill need to remember more than 1 of these
        // we store reference to buffer from dynamo
        // (this is like assigning a framebuffer resources for rendering)
        const SpacialInputBuffer & m_inputCache;
    };
}

#endif // FLY_CONTROL_RELAY_H