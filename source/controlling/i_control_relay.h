#ifndef I_CONTROL_RELAY_H
#define I_CONTROL_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "controlling/control_packet.h"
#include "core/cosmos.h"

namespace pleep
{
    class IControlRelay
    {
    public:
        // submittions are done, invoke controls
        // engage as in "engage the clutch"
        // each relay is different, enforce implementation to avoid confusion
        virtual void engage(double deltaTime) = 0;

        // Accept controllable entity
        void submit(ControlPacket data)
        {
            m_controlPacketQueue.push(data);
        }

    protected:
        // store all entities receiving controls this frame
        // this might not be necessary, are there any control schemes which are non-greedy?
        std::queue<ControlPacket> m_controlPacketQueue;

        // store input actions from dynamo (from ai or from input)
    };
}

#endif // I_CONTROL_RELAY_H