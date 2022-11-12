#ifndef SCRIPT_PACKET_H
#define SCRIPT_PACKET_H

//#include "intercession_pch.h"

#include "scripting/script_component.h"
#include "ecs/ecs_types.h"
#include "core/cosmos.h"

namespace pleep
{
    struct ScriptPacket
    {
        ScriptComponent& script;

        // Provide Cosmos-wide access
        Entity entity;
        Cosmos* owner;
    };
}

#endif // SCRIPT_PACKET_H