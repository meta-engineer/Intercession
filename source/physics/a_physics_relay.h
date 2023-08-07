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
    public:
        // Dynamo should provide shared broker for physics events to trigger callbacks
        // subclasses should be: using A_PhysicsRelay::A_PhysicsRelay;
        // virtual relay methods keep this abstract even without a protected constructor
        A_PhysicsRelay(std::shared_ptr<EventBroker> sharedBroker)
            : m_sharedBroker(sharedBroker)
        {
        }
        virtual ~A_PhysicsRelay() = default;

        // submittions are done, process entities
        virtual void engage(double deltaTime) = 0;

        // relays may be engaged multiply times per frame (fixed time step)
        // once done they should clear they're packets
        virtual void clear() = 0;

    protected:
        // broker given to us by Dynamo
        std::shared_ptr<EventBroker> m_sharedBroker = nullptr;
    };
}

#endif // A_PHYSICS_RELAY_H