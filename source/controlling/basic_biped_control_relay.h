#ifndef BASIC_BIPED_CONTROL_RELAY_H
#define BASIC_BIPED_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include <vector>
#include <glm/gtx/rotate_vector.hpp>
#include "controlling/biped_control_packet.h"

namespace pleep
{
    class BasicBipedControlRelay : public IControlRelay
    {
    public:
        BasicBipedControlRelay(const SpacialInputBuffer& actions)
            : m_inputCache(actions)
        {}
        
        void submit(BipedControlPacket data)
        {
            m_controlPackets.push_back(data);
        }

        void engage(double deltaTime) override
        {
            for (std::vector<BipedControlPacket>::iterator packet_it = m_controlPackets.begin(); packet_it != m_controlPackets.end(); packet_it++)
            {
                BipedControlPacket& data = *packet_it;
                
                // generate aim "heading" from aimOrientation and support vector
                // movement axes while airborn may depend on camera gimbal axis?
                const glm::mat3 aimRotation = glm::toMat4(data.controller.aimOrientation);
                const glm::vec3 aimHeading = glm::normalize(aimRotation * glm::vec3(0.0f, 0.0f, 1.0f));
                const glm::vec3 aimTangent = glm::normalize(glm::cross(aimHeading, data.controller.supportAxis));
                const glm::vec3 aimProjection = glm::normalize(glm::cross(data.controller.supportAxis, aimTangent));

                // read rotation actions and set controller orientation directly
                if (m_inputCache.actions.test(SpacialActions::rotatePitchUp))
                {
                    // rotate aimOrientation about aimTangent, using pitch input
                    data.controller.aimOrientation = glm::angleAxis(data.controller.verticalRotationFactor * m_mouseFactor * m_inputCache.actionVals.at(SpacialActions::rotatePitchUp) * (float)deltaTime, -aimTangent) * data.controller.aimOrientation;

                    // TODO: check if we have skipped past the singularity limit
                }
                if (m_inputCache.actions.test(SpacialActions::rotatePitchDown))
                {
                    data.controller.aimOrientation = glm::angleAxis(data.controller.verticalRotationFactor * m_mouseFactor * m_inputCache.actionVals.at(SpacialActions::rotatePitchDown) * (float)deltaTime, aimTangent) * data.controller.aimOrientation;
                }
                if (m_inputCache.actions.test(SpacialActions::rotateYawLeft))
                {
                    data.controller.aimOrientation = glm::angleAxis(data.controller.horizontalRotationFactor * m_mouseFactor * m_inputCache.actionVals.at(SpacialActions::rotateYawLeft) * (float)deltaTime, -data.controller.supportAxis) * data.controller.aimOrientation;
                }
                if (m_inputCache.actions.test(SpacialActions::rotateYawRight))
                {
                    data.controller.aimOrientation = glm::angleAxis(data.controller.horizontalRotationFactor * m_mouseFactor * m_inputCache.actionVals.at(SpacialActions::rotateYawRight) * (float)deltaTime, data.controller.supportAxis) * data.controller.aimOrientation;
                }

                // rotate transform towards aimHeading for debugging
                data.transform.orientation = data.controller.aimOrientation;

                // switch on current state, apply input accordingly
                // get "normalized" acceleration direction, relative to support axis
                // if we're on the ground we can only accelerate along the plane 
                //   represented by support axis. (or take a separate jump action)
                // if we're in the air we should allow omni-directional acceleration
                //   (support axis could be any direction depending on context)
                // so we'll have to use aimHeading to rationalize digital inputs

                if (data.controller.isGrounded)
                {
                    glm::vec3 targetGroundVelocity(0.0f);
                    // omit up/down from accelerations
                    if (m_inputCache.actions.test(SpacialActions::moveForward))
                    {
                        targetGroundVelocity += aimProjection * m_inputCache.actionVals.at(SpacialActions::moveForward);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveBackward))
                    {
                        targetGroundVelocity -= aimProjection * m_inputCache.actionVals.at(SpacialActions::moveBackward);
                    }

                    if (m_inputCache.actions.test(SpacialActions::moveLeft))
                    {
                        targetGroundVelocity -= aimTangent * m_inputCache.actionVals.at(SpacialActions::moveLeft);
                    }
                    if (m_inputCache.actions.test(SpacialActions::moveRight))
                    {
                        targetGroundVelocity += aimTangent * m_inputCache.actionVals.at(SpacialActions::moveRight);
                    }
                    if (targetGroundVelocity != glm::vec3(0.0f))
                        targetGroundVelocity = glm::normalize(targetGroundVelocity) * data.controller.groundMaxSpeed;
                    
                    //PLEEPLOG_DEBUG("targetGroundVelocity: " + std::to_string(targetGroundVelocity.x) + ", " + std::to_string(targetGroundVelocity.y) + ", " + std::to_string(targetGroundVelocity.z));

                    // derive ground velocity
                    const glm::vec3 groundVelocity = data.physics.velocity - (glm::dot(data.physics.velocity, data.controller.supportAxis) * data.controller.supportAxis);

                    // accelerate ground velocity towards target groudn velocity
                    // TODO: must only apply perpendicular to supportAxis
                    const glm::vec3 deltaGroundVelocity = targetGroundVelocity - groundVelocity;

                    // clamp deltaGroundVelocity with max acceleration

                    data.physics.acceleration += deltaGroundVelocity * data.controller.groundAcceleration;

                    // rotate entity towards its velocity

                    // treat up as "jump"
                    if (m_inputCache.actions.test(SpacialActions::moveUp))
                    {
                        //data.controller.isGrounded = false;
                        // temporarily disable leg collider?

                        // account for "known" fixed deltaTime?
                        data.physics.velocity += glm::vec3(0.0f, 0.3f, 0.0f);
                    }

                    // treat down as crouch
                    // change my "leg spring" collider size (ridelength)
                    if (m_inputCache.actions.test(SpacialActions::moveDown))
                    {
                        // account for "known" fixed deltaTime?
                        data.physics.velocity -= glm::vec3(0.0f, 0.3f, 0.0f);
                    }
                }


            }
            
            UNREFERENCED_PARAMETER(deltaTime);
        }
        
        // clear packets for next frame
        void clear() override
        {
            m_controlPackets.clear();
        }

    private:
        // we store reference to buffer from dynamo
        // (this is like assigning a framebuffer resources for rendering)
        const SpacialInputBuffer& m_inputCache;

        // convert mousemove input value
        // TODO: move this to ControlDynamo
        float m_mouseFactor = 0.2f;
        
        // store all entities receiving controls this frame and defer processing after all are submitted
        // this might not be necessary, are there any control schemes which are non-greedy?
        std::vector<BipedControlPacket> m_controlPackets;
    };
}

#endif // BASIC_BIPED_CONTROL_RELAY_H