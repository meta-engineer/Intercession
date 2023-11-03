#ifndef SPACETIME_COMPONENT_H
#define SPACETIME_COMPONENT_H

//#include "intercession_pch.h"
#include <cstdint>

namespace pleep
{
    // Spacetime variables are defined in their own component to optimize
    // not searching through "normal" entities when making timeline resolutions
    // SpacetimeComponents should be removed rather than just setting to
    //   default values to improve this efficiency

    // indication of how the entity interacts with the timestream
    enum TimestreamState
    {
        merged,         // Propogating state into past, and receiving new state from future
                        //   (whether or not there is any state being sent to it, like user entities)
        forked,         // Only propagating into the past, not receiving new state from future
                        //   indicates this entity is diverged from its future and needs to be resolved
        superposition,  // This entity is being resolved in a parallel cosmos in the past
                        //   its +1 causalchainlink entity should be in a forked state
    };

    // describes the state of the object related to time-travel
    // having no spacetime component implies default behaviour and state
    // this is used during timestream interception and intercession events
    struct SpacetimeComponent
    {
        TimestreamState timestreamState = TimestreamState::merged;

        // "time"stamp for when the timestreamState was last changed
        // used to trigger timestream interception resolutions
        uint16_t timestreamStateCoherency = 0;
    };
}

#endif // SPACETIME_COMPONENT_H