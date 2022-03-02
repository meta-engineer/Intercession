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
                    float disp = 5.0f * (float)deltaTime;
                    float rot  = 1.0f * (float)deltaTime;
                    glm::vec3 gimbalUp = glm::vec3(0.0f, 1.0f, 0.0);
                    
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
                        transform.origin += glm::cross(direction, gimbalUp) * disp * m_inputCache.actionVals.at(SpacialActions::moveLeft);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveRight)
                    && !m_inputCache.actions.test(SpacialActions::moveLeft))
                    {
                        transform.origin -= glm::cross(direction, gimbalUp) * disp * m_inputCache.actionVals.at(SpacialActions::moveRight);
                    }

                    if (m_inputCache.actions.test(SpacialActions::moveUp)
                    && !m_inputCache.actions.test(SpacialActions::moveDown))
                    {
                        // always go gimbal_up regardless of rotation
                        transform.origin += gimbalUp * disp * m_inputCache.actionVals.at(SpacialActions::moveUp);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveDown)
                    && !m_inputCache.actions.test(SpacialActions::moveUp))
                    {
                        // always go gimbal_up regardless of rotation
                        transform.origin -= gimbalUp * disp * m_inputCache.actionVals.at(SpacialActions::moveDown);
                    }

                    
                    if (m_inputCache.actions.test(SpacialActions::rotatePitchUp)
                    && !m_inputCache.actions.test(SpacialActions::rotatePitchDown))
                    {
                        transform.rotation.x += rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp);
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotatePitchDown)
                    && !m_inputCache.actions.test(SpacialActions::rotatePitchUp))
                    {
                        transform.rotation.x -= rot * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown);
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotateYawLeft)
                    && !m_inputCache.actions.test(SpacialActions::rotateYawRight))
                    {
                        transform.rotation.y -= rot * m_inputCache.actionVals.at(SpacialActions::rotateYawLeft);
                    }
                    if (m_inputCache.actions.test(SpacialActions::rotateYawRight)
                    && !m_inputCache.actions.test(SpacialActions::rotateYawLeft))
                    {
                        transform.rotation.y += rot * m_inputCache.actionVals.at(SpacialActions::rotateYawRight);
                    }
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