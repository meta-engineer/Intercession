#ifndef CONTROL_PACKET_H
#define CONTROL_PACKET_H

//#include "intercession_pch.h"
#include "controlling/control_component.h"


namespace pleep
{
    // Forward declare: Relay needs dynamic access to ecs for other components
    class Cosmos;

    // ControlPacket only contains the entities control component
    // as we don't know which components will need to be modified
    // ControlDynamo/Relays will store "actions" in control component reference
    // to be processed by synchro (who can access ECS)
    struct ControlPacket
    {
        ControlComponent& controller;
        Entity controllee;
        Cosmos* owner;
    };
}

#endif // CONTROL_PACKET_H
