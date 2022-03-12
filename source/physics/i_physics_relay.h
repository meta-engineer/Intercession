#ifndef I_PHYSICS_RELAY_H
#define I_PHYSICS_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "physics/physics_packet.h"

namespace pleep
{
    class IPhysicsRelay
    {
    public:
        // submittions are done, process entities
        virtual void engage(double deltaTime) = 0;
        
        // different relay types might want to store packets in different structures
        virtual void submit(PhysicsPacket data) = 0;

    private:
        
    };
}

#endif // I_PHYSICS_RELAY_H