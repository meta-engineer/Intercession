#ifndef TIMEJUMP_CONDITIONS_H
#define TIMEJUMP_CONDITIONS_H

//#include "intercession_pch.h"
#include "physics/transform_component.h"

namespace pleep
{
    // a "condensed" version of a generic entity's state which determines if their 
    // state during a jump departure is equivalent to another
    struct TimejumpConditions
    {
        // unique id for this jump (departure and arrival)
        // not used for comparison
        uint32_t tripId;

        // extracted from TransformComponent's origin
        glm::vec3 origin;
        
        // determine if coniditons are "Congruent" not necessarily equal
        friend bool operator==(const TimejumpConditions& lhs, const TimejumpConditions& rhs)
        {
            // origin's should be equal "enough"
            return (glm::length2(lhs.origin - rhs.origin) <= 2.0f);     //sqrt(2) ~= 1.4
        }
    };

    // DESTRUCTIVELY extracts data from a jump message to create conditions for congruency
    // Assumes data is fully serialized and packed with params
    // cosmos is relative to components in data
    inline TimejumpConditions extract_jump_conditions(EventMessage& data, std::shared_ptr<Cosmos> cosmos)
    {
        events::network::JUMP_params jumpInfo;
        data >> jumpInfo;
        
        TimejumpConditions jumpConditions { jumpInfo.tripId };
        
        const ComponentType transformType = cosmos->get_component_type<TransformComponent>();
        for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
        {
            if (!jumpInfo.sign.test(i))
            {
                continue;
            }
            if (transformType == i)
            {
                TransformComponent jumperTransform;
                data >> jumperTransform;
                jumpConditions.origin = jumperTransform.origin;
            }
            else
            {
                cosmos->discard_single_component(i, data);
            }
        }

        return jumpConditions;
    }
}

#endif // TIMEJUMP_CONDITIONS_H