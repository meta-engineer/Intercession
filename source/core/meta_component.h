#ifndef META_COMPONENT_H
#define META_COMPONENT_H

//#include "intercession_pch.h"
#include <string>
#include "ecs/ecs_types.h"

namespace pleep
{
    // Misc per-entity meta data
    struct MetaComponent
    {
        // searchable, readable identifier (watch for collisions?)
        std::string name;
    };
}

#endif // META_COMPONENT_H