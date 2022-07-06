#ifndef PHYSICS_DYNAMO_H
#define PHYSICS_DYNAMO_H

//#include "intercession_pch.h"

// external
#include <memory>

#include "core/a_dynamo.h"
#include "events/event_broker.h"
#include "physics/physics_packet.h"
#include "physics/euler_physics_relay.h"
#include "physics/collider_packet.h"
#include "physics/collision_physics_relay.h"

namespace pleep
{
    class PhysicsDynamo : public A_Dynamo
    {
    public:
        PhysicsDynamo(EventBroker* sharedBroker);
        ~PhysicsDynamo();

        // pass in all physics information
        void submit(PhysicsPacket data);

        // pass in all collider information
        void submit(ColliderPacket data);

        // process physics/collision packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // should I store entities for optimal spacial partitioning?

        // RELAY STEP 1
        std::unique_ptr<EulerPhysicsRelay> m_motionStep;

        // RELAY STEP 2
        std::unique_ptr<CollisionPhysicsRelay> m_collisionStep;
    };
}

#endif // PHYSICS_DYNAMO_H