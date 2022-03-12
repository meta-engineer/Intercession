#ifndef CONTROL_PACKET_H
#define CONTROL_PACKET_H

//#include "intercession_pch.h"
#include "controlling/control_component.h"


namespace pleep
{
    // Forward declare: Relay needs dynamic access to ecs for other components
    class Cosmos;

    // As we don't know which components will need to be modified
    // ControlPacket will contain Cosmos that this entity belongs to
    // so it can access ECS
    struct ControlPacket
    {
        ControlComponent& controller;
        Entity controllee;
        Cosmos* owner;
    };
}

#endif // CONTROL_PACKET_H
