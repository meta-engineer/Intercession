#include "physics_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    PhysicsDynamo::PhysicsDynamo(std::shared_ptr<EventBroker> sharedBroker)
        : A_Dynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Start Physics pipeline setup");
        
        // setup relays
        m_motionStep = std::make_unique<EulerPhysicsRelay>(m_sharedBroker);
        m_collisionStep = std::make_unique<CollisionPhysicsRelay>(m_sharedBroker);

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
        // BVH is now built? process all physics relays
        
        // motion first
        m_motionStep->engage(deltaTime);
        // then detect and resolve collision
        m_collisionStep->engage(deltaTime);
    }
    
    void PhysicsDynamo::reset_relays()
    {
        // after 1+ integration steps clear relays of entities
        m_motionStep->clear();
        m_collisionStep->clear();
    }
}