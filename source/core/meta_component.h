#ifndef META_COMPONENT_H
#define META_COMPONENT_H

//#include "intercession_pch.h"
#include <string>
#include "ecs/ecs_types.h"

namespace pleep
{

    // General per-entity meta data
    struct MetaComponent
    {
        // searchable, readable identifier (watch for collisions?)
        std::string name;
        // unique ID coordinated by timeline cluster to associate Entities between timeslices
        // this CANNOT be folded together with entity id becuase 2 different entities can have
        // this same timeline id (one has jumped back)
        Entity timelineEntityId = NULL_ENTITY;
    };
}

#endif // META_COMPONENT_H