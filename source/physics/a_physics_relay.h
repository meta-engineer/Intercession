#ifndef A_PHYSICS_RELAY_H
#define A_PHYSICS_RELAY_H

//#include "intercession_pch.h"

namespace pleep
{
    // Abstract base class for physics processes
    // Not all subclasses use collision packets and/or physics packets 
    // so they can implement submit() methods as needed
    class A_PhysicsRelay
    {
    protected:
        A_PhysicsRelay() = default;
    public:
        virtual ~A_PhysicsRelay() = default;

        // submittions are done, process entities
        virtual void engage(double deltaTime) = 0;

        // relays may be engaged multiply times per frame (fixed time step)
        // once done they should clear they're packets
        virtual void clear() = 0;
    };
}

#endif // A_PHYSICS_RELAY_H