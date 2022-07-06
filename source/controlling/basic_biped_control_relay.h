#ifndef BASIC_BIPED_CONTROL_RELAY_H
#define BASIC_BIPED_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include <vector>
#include <glm/gtx/rotate_vector.hpp>
#include "controlling/biped_control_packet.h"

namespace pleep
{
    class BasicBipedControlRelay : public A_ControlRelay
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
                
                // derive ground velocity perpendicular to support axis
                const glm::vec3 planarVelocity = data.physics.velocity - (glm::dot(data.physics.velocity, data.controller.supportAxis) * data.controller.supportAxis);

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

                // rotate entity towards its velocity
                // generate quat from velocity and support axis?
                // magic number to calibrate turn rate
                const float turnFactor = 8.0f;
                if (glm::length2(planarVelocity) > 0.01f)
                {
                    glm::vec3 normalVelocity = glm::normalize(planarVelocity);
                    glm::quat velocityOrientation = glm::quat(glm::vec3(0.0f,0.0f,1.0f), normalVelocity);
                    // slerp from current orientation
                    //data.transform.orientation = velocityOrientation;
                    data.transform.orientation = glm::slerp(data.transform.orientation, velocityOrientation, glm::min((float)deltaTime * turnFactor, 1.0f));
                }

                // switch on current state, apply input accordingly
                // get "normalized" acceleration direction, relative to support axis
                // if we're on the ground we can only accelerate along the plane 
                //   represented by support axis. (or take a separate jump action)
                // if we're in the air we should allow omni-directional acceleration
                //   (support axis could be any direction depending on context)
                // so we'll have to use aimHeading to rationalize digital inputs

                glm::vec3 targetInputVector(0.0f);
                // omit up/down from accelerations
                if (m_inputCache.actions.test(SpacialActions::moveForward))
                {
                    targetInputVector += aimProjection * m_inputCache.actionVals.at(SpacialActions::moveForward);
                }
                if (m_inputCache.actions.test(SpacialActions::moveBackward))
                {
                    targetInputVector -= aimProjection * m_inputCache.actionVals.at(SpacialActions::moveBackward);
                }
                if (m_inputCache.actions.test(SpacialActions::moveLeft))
                {
                    targetInputVector -= aimTangent * m_inputCache.actionVals.at(SpacialActions::moveLeft);
                }
                if (m_inputCache.actions.test(SpacialActions::moveRight))
                {
                    targetInputVector += aimTangent * m_inputCache.actionVals.at(SpacialActions::moveRight);
                }
                // wait to normalize, different biped states could want to add to it?

                // update jump state
                // jumping system works on a cycle of 4 properties
                //      isGrounded  -> enable jump input
                //      jump input  -> add vertical velocity, disable legs, start cooldown
                //      cooldown    -> enable legs
                //      legs        -> set isGrounded   (back to top)
                if (m_jumpCooldownRemaining > 0)
                {
                    m_jumpCooldownRemaining -= deltaTime;
                }
                else
                {
                    // re-enable leg collider
                    data.legCollider.isActive = true;
                }

                if (data.controller.isGrounded)
                {
                    if (targetInputVector != glm::vec3(0.0f))
                        targetInputVector = glm::normalize(targetInputVector);
                    glm::vec3 targetGroundVelocity(0.0f);
                    targetGroundVelocity = targetInputVector * data.controller.groundMaxSpeed;
                    
                    //PLEEPLOG_DEBUG("targetGroundVelocity: " + std::to_string(targetGroundVelocity.x) + ", " + std::to_string(targetGroundVelocity.y) + ", " + std::to_string(targetGroundVelocity.z));

                    // accelerate ground velocity towards target ground velocity
                    // TODO: Should entities moving faster than their max slow down? Or should they trip/soft knockdown(ukemi)/not slow down if they slide?
                    const glm::vec3 deltaGroundVelocity = targetGroundVelocity - planarVelocity;

                    // TODO: acceleration should not be based on max ground speed
                    data.physics.acceleration += deltaGroundVelocity * data.controller.groundAcceleration;

                    // treat up as "jump"
                    const bool jump = m_inputCache.actions.test(SpacialActions::moveUp);
                    if (jump && m_jumpCooldownRemaining <= 0)
                    {
                        //data.controller.isGrounded = false;
                        // temporarily disable leg collider?
                        data.legCollider.isActive = false;
                        m_jumpCooldownRemaining = m_jumpCooldownTime;

                        // account for "known" fixed deltaTime?
                        data.physics.velocity += data.controller.supportAxis * -4.0f;
                    }

                    // treat down as crouch
                    // change my "leg spring" collider size (ridelength)
                    if (m_inputCache.actions.test(SpacialActions::moveDown))
                    {
                        // account for "known" fixed deltaTime?
                        data.physics.velocity -= glm::vec3(0.0f, 0.3f, 0.0f);
                    }

                    //unset grounded state, to be set by collider(s)
                    data.controller.isGrounded = false;
                    // disable setting isGrounded for some amount of time?
                    // disable legs collider to prevent spring damping?
                }
                // not on ground
                else
                {
                    if (targetInputVector != glm::vec3(0.0f))
                        targetInputVector = glm::normalize(targetInputVector);
                    glm::vec3 targetAirVelocity(0.0f);
                    targetAirVelocity = targetInputVector * data.controller.airMaxSpeed;

                    const glm::vec3 deltaAirVelocity = targetAirVelocity - planarVelocity;

                    // TODO: acceleration shouldn't be based on max air speed (this causes unintentional "drag" force)                    
                    data.physics.acceleration += deltaAirVelocity * data.controller.airAcceleration;
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

        // relay will track input start vs press
        bool m_lastJump = false;
        double m_jumpCooldownTime = 0.2; // if fixed update is 0.05, this means 4 frames
        double m_jumpCooldownRemaining;
        
        // store all entities receiving controls this frame and defer processing after all are submitted
        // this might not be necessary, are there any control schemes which are non-greedy?
        std::vector<BipedControlPacket> m_controlPackets;
    };
}

#endif // BASIC_BIPED_CONTROL_RELAY_H