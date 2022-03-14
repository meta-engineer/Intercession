#include "physics_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    PhysicsDynamo::PhysicsDynamo(EventBroker* sharedBroker)
        : IDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Physics pipeline");
        
        // setup relays
        m_motionStep = std::make_unique<VerletPhysicsRelay>();
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
        m_collisionStep->submit(data);
    }
    
    void PhysicsDynamo::run_relays(double deltaTime) 
    {
        // quadtree is now built? process all physical processes
        
        // motion first
        m_motionStep->engage(deltaTime);
        // then detect and resolve collision
        m_collisionStep->engage(deltaTime);
    }
}