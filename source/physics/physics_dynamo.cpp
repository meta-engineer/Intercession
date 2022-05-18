#include "physics_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    PhysicsDynamo::PhysicsDynamo(EventBroker* sharedBroker)
        : IDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Physics pipeline");
        
        // setup relays
        m_motionStep = std::make_unique<EulerPhysicsRelay>();
        m_collisionStep = std::make_unique<CollisionPhysicsRelay>();

        PLEEPLOG_TRACE("Done Physics pipeline setup");
    }
    
    PhysicsDynamo::~PhysicsDynamo() 
    {
    }
    
    void PhysicsDynamo::submit(PhysicsPacket data) 
    {
        // dispatch to motion integration relays
        // *All packets to improved euler relay
        m_motionStep->submit(data);
    }
    
    void PhysicsDynamo::submit(ColliderPacket data)
    {
        // build BVH in dynamo? or pass BVH container to a broad phase relay?

        // *All packets to narrow phase for now
        m_collisionStep->submit(data);
    }
    
    void PhysicsDynamo::run_relays(double deltaTime) 
    {
        // BVH is now built? process all physical processes
        
#define PHYSICS_FIXED_TIMESTEP
#ifdef PHYSICS_FIXED_TIMESTEP
        size_t stepsTaken = 0;
        m_timeRemaining += deltaTime;
        while (m_timeRemaining >= m_fixedTimeStep && stepsTaken <= m_maxSteps)
        {
            m_timeRemaining -= m_fixedTimeStep;
            stepsTaken++;

            // motion first
            m_motionStep->engage(m_fixedTimeStep);
            // then detect and resolve collision
            m_collisionStep->engage(m_fixedTimeStep);
        }
#else
        // motion first
        m_motionStep->engage(deltaTime);
        // then detect and resolve collision
        m_collisionStep->engage(deltaTime);
#endif // PHYSICS_FIXED_TIMESTEP

        // after fixed timesteps clear relays
        m_motionStep->clear();
        m_collisionStep->clear();
    }
}