#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H

//#include "intercession_pch.h"
#include "physics/collider.h"

namespace pleep
{
    constexpr size_t COLLIDERS_PER_ENTITY = 2;

    struct ColliderComponent
    {
        // Give every Entity a static array of a few colliders
        // thus you can have multiple of the same "component"
        // the synchro will submit each collider (if active)
        Collider colliders[COLLIDERS_PER_ENTITY];
    };
}

#endif // COLLIDER_COMPONENT_H