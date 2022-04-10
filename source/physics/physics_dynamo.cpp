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
        // continue to build quad tree?

        // dispatch to physics relays
        m_motionStep->submit(data);
        
        if (data.physics.collider != nullptr)
            m_collisionStep->submit(data);
    }
    
    void PhysicsDynamo::run_relays(double deltaTime) 
    {
        // quadtree is now built? process all physical processes

#define FIXED_TIME_STEP
#ifdef FIXED_TIME_STEP
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
#endif // FIXED_TIME_STEP

        // after fixed timesteps clear relays
        m_motionStep->clear();
        m_collisionStep->clear();
    }
}