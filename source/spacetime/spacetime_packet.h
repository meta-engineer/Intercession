#ifndef SPACETIME_PACKET_H
#define SPACETIME_PACKET_H

//#include "intercession_pch.h"

#include "spacetime/spacetime_component.h"

namespace pleep
{
    struct SpacetimePacket
    {
        Entity entity;
        SpacetimeComponent& spacetime;
    };
}

#endif // SPACETIME_PACKET_H