#ifndef BIPED3P_CAMERA_CONTROL_RELAY_H
#define BIPED3P_CAMERA_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include <vector>
#include "controlling/a_control_relay.h"
#include "controlling/spacial_input_buffer.h"
#include "controlling/camera_control_packet.h"
#include "physics/physics_component.h"
#include "core/cosmos.h"
#include "controlling/biped_control_component.h"

namespace pleep
{
    class Biped3pCameraControlRelay : public A_ControlRelay
    {
    public:
        Biped3pCameraControlRelay(const SpacialInputBuffer& actions)
            : m_inputCache(actions)
        {}

        void submit(CameraControlPacket data)
        {
            m_controlPackets.push_back(data);
        }
        
        void engage(double deltaTime) override
        {
            // check target entity for biped controller and align with the biped
            
            for (std::vector<CameraControlPacket>::iterator packet_it = m_controlPackets.begin(); packet_it != m_controlPackets.end(); packet_it++)
            {
                CameraControlPacket& data = *packet_it;

                if (data.controller.target == pleep::NULL_ENTITY)
                {
                    continue; // do nothing without target?
                }

                // get entire controller or just extract what's necessary?
                BipedControlComponent* targetBiped = nullptr;
                TransformComponent* targetTransform = nullptr;
                try
                {
                    targetBiped = &(data.owner->get_component<BipedControlComponent>(data.controller.target));
                    targetTransform = &(data.owner->get_component<TransformComponent>(data.controller.target));
                }
                catch (const std::range_error& err)
                {
                    UNREFERENCED_PARAMETER(err);
                    PLEEPLOG_WARN(err.what());
                    PLEEPLOG_WARN("Could not fetch a Biped Controller on target entity " + std::to_string(data.controllee) + " , clearing and skipping");
                    data.controller.target = NULL_ENTITY;
                    continue;
                }

                // TEMP: set my orientation to match directly to aim
                data.transform.orientation = targetBiped->aimOrientation;
                const glm::mat3 aimRotation = glm::toMat4(targetBiped->aimOrientation);
                const glm::vec3 aimHeading = glm::normalize(aimRotation * glm::vec3(0.0f, 0.0f, 1.0f));
                const glm::vec3 aimTangent = glm::normalize(glm::cross(aimHeading, targetBiped->supportAxis));
                const glm::vec3 aimUp      = glm::normalize(glm::cross(aimHeading, aimTangent));

                data.transform.origin = targetTransform->origin;
                data.transform.origin -= aimHeading * data.controller.range;
                data.transform.origin += aimUp * data.controller.range * 0.2f;

                UNREFERENCED_PARAMETER(deltaTime);
            }
        }
        
        // clear packets for next frame
        void clear() override
        {
            m_controlPackets.clear();
        }

    private:
        // reference to data updated in dynamo
        // this could be copied each update for safety, but this is probably technically faster
        const SpacialInputBuffer& m_inputCache;
        
        // store all entities receiving controls this frame and defer processing after all are submitted
        // this might not be necessary, are there any control schemes which are non-greedy?
        std::vector<CameraControlPacket> m_controlPackets;
    };
}

#endif // BIPED3P_CAMERA_CONTROL_RELAY_H