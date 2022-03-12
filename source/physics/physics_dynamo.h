#ifndef PHYSICS_DYNAMO_H
#define PHYSICS_DYNAMO_H

//#include "intercession_pch.h"

// external
#include <memory>

#include "core/i_dynamo.h"
#include "events/event_broker.h"
#include "physics/physics_packet.h"

namespace pleep
{
    class PhysicsDynamo : public IDynamo
    {
    public:
        PhysicsDynamo(EventBroker* sharedBroker);
        ~PhysicsDynamo();

        // read entities into spacial partition
        void submit(PhysicsPacket data);

        // process render command queue
        void run_relays(double deltaTime) override;

    private:
        // internal entity storage for optimal spacial partitioning
        //QuadTree
    };
}

#endif // PHYSICS_DYNAMO_H