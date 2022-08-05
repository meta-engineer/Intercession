#ifndef I_NETWORK_DYNAMO_H
#define I_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include "core/a_dynamo.h"

namespace pleep
{
    // Instead of operating like a normal dynamo (where a synchro submits entity packets to be updated)
    // it might make more sense to give the network dynamo direct access to the ecs and it can update
    // only the entities with new data (not all entities may have new data to update and will be skipped)
    // Entities may need to have a NetworkComponent to maintain some states/persistence
    class I_NetworkDynamo : public A_Dynamo
    {
    protected:
        // passthrough to A_Dynamo (can't call A_Dynamo's constructor directly from 2 layers deep)
        I_NetworkDynamo(EventBroker* sharedBroker) 
            : A_Dynamo(sharedBroker)
        {}

    public:
        // provide access to entire ecs to apply updates to entities only as needed
        //virtual void submit();
    };
}

#endif // I_NETWORK_DYNAMO_H