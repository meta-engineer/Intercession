#ifndef PHYSICS_DYNAMO_H
#define PHYSICS_DYNAMO_H

//#include "intercession_pch.h"

// external
#include <memory>

#include "core/i_dynamo.h"
#include "events/event_broker.h"
#include "physics/physics_packet.h"
#include "physics/verlet_physics_relay.h"
#include "physics/collision_physics_relay.h"

namespace pleep
{
    class PhysicsDynamo : public IDynamo
    {
    public:
        PhysicsDynamo(EventBroker* sharedBroker);
        ~PhysicsDynamo();

        // pass in all physics information
        void submit(PhysicsPacket data);

        // process render command queue
        void run_relays(double deltaTime) override;

    private:
        // should I store entities for optimal spacial partitioning?

        // RELAY STEP 1
        std::unique_ptr<VerletPhysicsRelay> m_motionStep;

        // RELAY STEP 2
        std::unique_ptr<CollisionPhysicsRelay> m_collisionStep;
    };
}

#endif // PHYSICS_DYNAMO_H