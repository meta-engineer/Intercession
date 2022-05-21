#ifndef PHYSICS_DYNAMO_H
#define PHYSICS_DYNAMO_H

//#include "intercession_pch.h"

// external
#include <memory>

#include "core/i_dynamo.h"
#include "events/event_broker.h"
#include "physics/physics_packet.h"
#include "physics/euler_physics_relay.h"
#include "physics/collider_packet.h"
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

        // pass in all collider information
        void submit(ColliderPacket data);

        // process render command queue
        void run_relays(double deltaTime) override;

    private:
        // subroutine to engage all relays in my configuration
        void engage_all(double deltaTime);

        // should I store entities for optimal spacial partitioning?

        // RELAY STEP 1
        std::unique_ptr<EulerPhysicsRelay> m_motionStep;

        // RELAY STEP 2
        std::unique_ptr<CollisionPhysicsRelay> m_collisionStep;

        // Fixed timestep for stability
        // 200hz?
        const double m_fixedTimeStep = 0.005;
        // mechanism for tracking how many timesteps to process
        double m_timeRemaining = 0.0;
        // max number of iterations to catchup before letting system progress/respond
        const size_t m_maxSteps = 30;
    };
}

#endif // PHYSICS_DYNAMO_H