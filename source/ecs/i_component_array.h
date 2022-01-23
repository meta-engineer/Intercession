#ifndef I_COMPONENT_ARRAY_H
#define I_COMPONENT_ARRAY_H

//#include "intercession_pch.h"
#include "ecs_types.h"

namespace pleep
{
    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;
        
        // component manager needs to dispatch anonymously 
        // to all component arrays to update their mappings
        
        // remove component from entity
        // non-strict usage, does nothing if component does not exist
        virtual void clear_for(Entity entity) = 0;
    };
}

#endif // I_COMPONENT_ARRAY_H