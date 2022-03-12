#include "physics_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    PhysicsDynamo::PhysicsDynamo(EventBroker* sharedBroker)
        : IDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Physics pipeline");

        PLEEPLOG_TRACE("Done Physics pipeline setup");
    }
    
    PhysicsDynamo::~PhysicsDynamo() 
    {
    }
    
    void PhysicsDynamo::submit(PhysicsPacket data) 
    {
        // continue to build quad tree
        UNREFERENCED_PARAMETER(data);
    }
    
    void PhysicsDynamo::run_relays(double deltaTime) 
    {
        // quadtree is now built process all physical processes
        UNREFERENCED_PARAMETER(deltaTime);
        
    }
}