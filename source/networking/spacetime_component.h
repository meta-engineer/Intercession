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

    // indication of how the entity uses the timestream
    // could this be represended by a 2 bit bitset?
    enum TimestreamState
    {
        body,       // Receiving from future, and propogating into past.
                    //   this is the normal, default state
                    //   also used for entities which have no future (user controlled)
        head,       // Only propagating into the past.
                    //   indicates this entity is detached in the future
        //tail,     // Only receiving from future.
                    //   use detached instead
        detached,   // Not linked to past or future timestreams.
                    //   aka "superposition", this entity's past is being modified
    };

    // describes the state of the object related to time-travel
    // having no spacetime component implies default behaviour and state
    // this is used during timestream interception and intercession events
    struct SpacetimeComponent
    {
        TimestreamState timestreamState = TimestreamState::body;

        // "time"stamp for when the timestream_state was last changed
        // used to trigger timestream interception resolutions
        uint16_t timestreamStateCoherency = 0;

    };
}

#endif // SPACETIME_COMPONENT_H