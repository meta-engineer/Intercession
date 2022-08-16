#ifndef COSMOS_ACCESS_PACKET_H
#define COSMOS_ACCESS_PACKET_H

//#include "intercession_pch.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    // Forward declare: Relay needs dynamic access to ecs for other components
    class Cosmos;
    
    // Contains only the Cosmos reference from the submitting synchro
    // Used for dynamos which need to select entities more dynamically than an ECS 
    // synchro signature would allow (I.E. from the other side of a network)
    struct CosmosAccessPacket
    {
        Cosmos* owner = nullptr;
    };
}

#endif // COSMOS_ACCESS_PACKET_H