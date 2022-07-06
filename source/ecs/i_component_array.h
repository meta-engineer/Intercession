#ifndef I_COMPONENT_ARRAY_H
#define I_COMPONENT_ARRAY_H

//#include "intercession_pch.h"
#include "ecs_types.h"

namespace pleep
{
    // Interface for component registry to clear entity data on all templated component arrays
    class I_ComponentArray
    {
    protected:
        I_ComponentArray() = default;
    public:
        virtual ~I_ComponentArray() = default;
        
        // component manager needs to dispatch anonymously 
        // to all component arrays to update their mappings
        
        // remove component from entity
        // non-strict usage, does nothing if component does not exist
        virtual void clear_data_for(Entity entity) = 0;
    };
}

#endif // I_COMPONENT_ARRAY_H