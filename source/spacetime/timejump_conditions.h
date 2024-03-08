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
}

#endif // TIMEJUMP_CONDITIONS_H