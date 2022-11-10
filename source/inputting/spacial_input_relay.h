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
            
            newSpacialInput.set(SpacialActions::rotatePitch, m_sharedBuffer.twoDimAnalog[0][1] != 0.0, m_sharedBuffer.twoDimAnalog[0][1]);
            newSpacialInput.set(SpacialActions::rotateYaw, m_sharedBuffer.twoDimAnalog[0][0] != 0.0, m_sharedBuffer.twoDimAnalog[0][0]);

            newSpacialInput.set(SpacialActions::action0, m_sharedBuffer.digitalState[GLFW_MOUSE_BUTTON_1], m_sharedBuffer.digitalEdge[GLFW_MOUSE_BUTTON_1]);


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