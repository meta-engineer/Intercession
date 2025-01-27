#ifndef SPACIAL_INPUT_RELAY_H
#define SPACIAL_INPUT_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "inputting/a_input_relay.h"
#include "inputting/spacial_input_packet.h"

namespace pleep
{
    class SpacialInputRelay : A_InputRelay
    {
    public:
        SpacialInputRelay(const RawInputBuffer& sharedBuffer)
            : A_InputRelay(sharedBuffer)
        {}
        
        void submit(SpacialInputPacket data)
        {
            m_inputPackets.push_back(data);
        }
        
        void engage(double deltaTime) override
        {
            UNREFERENCED_PARAMETER(deltaTime);

            // does new SpacialInputComponent state depend on the previous one?
            // or will all submitted components receive the same values
            // build SpacialInputComponent from m_sharedBuffer
            SpacialInputComponent newSpacialInput;

            // maintain map of buffer values to component values?
            // e.g. value 'w' -> moveParallel
            // then the map could be changed to rebind keys

            // TEMP naive solution
            double p = (m_sharedBuffer.digitalState[GLFW_KEY_W] ?  1.0 : 0.0) 
                     + (m_sharedBuffer.digitalState[GLFW_KEY_S] ? -1.0 : 0.0);
            newSpacialInput.set(SpacialActions::moveParallel, p!=0.0 ? true : false, p);
            double h = (m_sharedBuffer.digitalState[GLFW_KEY_D] ?  1.0 : 0.0) 
                     + (m_sharedBuffer.digitalState[GLFW_KEY_A] ? -1.0 : 0.0);
            newSpacialInput.set(SpacialActions::moveHorizontal, h!=0.0 ? true : false, h);
            double v = (m_sharedBuffer.digitalState[GLFW_KEY_SPACE] ?  1.0 : 0.0) 
                     + (m_sharedBuffer.digitalState[GLFW_KEY_C] ? -1.0 : 0.0);
            newSpacialInput.set(SpacialActions::moveVertical, v!=0.0 ? true : false, v);
            
            newSpacialInput.set(SpacialActions::rotatePitch, 
                m_sharedBuffer.twoDimAnalog[0][1] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.twoDimAnalog[0][1]
            );
            newSpacialInput.set(SpacialActions::rotateYaw, 
                m_sharedBuffer.twoDimAnalog[0][0] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.twoDimAnalog[0][0]
            );
            // use mouse wheel Y as roll?
            newSpacialInput.set(SpacialActions::rotateRoll, 
                m_sharedBuffer.twoDimAnalog[2][1] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.twoDimAnalog[2][1]
            );

            newSpacialInput.set(SpacialActions::raycastX,
                m_sharedBuffer.twoDimAnalog[1][0] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.twoDimAnalog[1][0]
            );
            newSpacialInput.set(SpacialActions::raycastY,
                m_sharedBuffer.twoDimAnalog[1][1] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.twoDimAnalog[1][1]
            );

            newSpacialInput.set(SpacialActions::targetX,
                m_sharedBuffer.threeDimAnalog[0][0] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.threeDimAnalog[0][0]
            );
            newSpacialInput.set(SpacialActions::targetY,
                m_sharedBuffer.threeDimAnalog[0][1] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.threeDimAnalog[0][1]
            );
            newSpacialInput.set(SpacialActions::targetZ,
                m_sharedBuffer.threeDimAnalog[0][2] != std::numeric_limits<double>::max(), 
                m_sharedBuffer.threeDimAnalog[0][2]
            );


            newSpacialInput.set(SpacialActions::action0, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_1], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_1]);
            newSpacialInput.set(SpacialActions::action1, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_2], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_2]);
            newSpacialInput.set(SpacialActions::action2, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_3], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_3]);
            newSpacialInput.set(SpacialActions::action3, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_4], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_4]);
            newSpacialInput.set(SpacialActions::action4, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_5], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_5]);


            for (std::vector<SpacialInputPacket>::iterator packet_it = m_inputPackets.begin(); packet_it != m_inputPackets.end(); packet_it++)
            {
                //SpacialInputPacket& data = *packet_it;

                // copy this frame's inputs into ecs
                (*packet_it).input = newSpacialInput;
            }
        }

        void clear() override
        {
            m_inputPackets.clear();
        }

    private:
        std::vector<SpacialInputPacket> m_inputPackets;
    };
}

#endif // SPACIAL_INPUT_RELAY_H