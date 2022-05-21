#ifndef I_CONTROL_RELAY_H
#define I_CONTROL_RELAY_H

//#include "intercession_pch.h"

namespace pleep
{
    class IControlRelay
    {
    public:
        // submittions are done, invoke controls
        // engage as in "engage the clutch"
        // each relay is different, enforce implementation to avoid confusion
        virtual void engage(double deltaTime) = 0;
        
        // relays may be engaged multiply times per frame (fixed time step)
        // once done they should clear they're packets
        virtual void clear() = 0;

        // each control relay will have a container of different packet types
        // and store a reference to different input actions from dynamo (from npc or from input)
    };
}

#endif // I_CONTROL_RELAY_H