#ifndef I_COMPONENT_ARRAY_H
#define I_COMPONENT_ARRAY_H

//#include "intercession_pch.h"
#include "ecs_types.h"
#include "events/event_types.h"

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

        // add default constructed component to entity
        virtual void emplace_data_for(Entity entity) = 0;

        // safely check if there is any data for given entity
        virtual bool has_data_for(Entity entity) = 0;

        // Push component data into msg
        // non-strict usage, does nothing if component does not exist
        virtual void serialize_data_for(Entity entity, EventMessage& msg) = 0;

        // Pop component data from msg and overwrite;
        // non-strict usage, does nothing if component does not exist
        virtual void deserialize_data_for(Entity entity, EventMessage& msg) = 0;
    };
}

#endif // I_COMPONENT_ARRAY_H