#ifndef TIMESTREAM_STATE_H
#define TIMESTREAM_STATE_H

//#include "intercession_pch.h"
#include <cstdint>

namespace pleep
{

    // indication of how the entity interacts with the timestream
    enum TimestreamState
    {
        merged = 0,     // Propogating state into past, and receiving new state from future
                        //   (whether or not there is any state being sent to it, like user entities)
        merging,        // This entity's FUTURE is currently being resolved by a parallel cosmos
                        //   will convert to merged once future timestream is spliced
        forked,         // Only propagating into the past, not receiving new state from future
                        //   indicates this entity is diverged from its future and needs to be resolved
        forking,        // Considered as forked for propogation, but awaiting timeout before
                        //   becoming fully "forked" and available for resolution
        superposition,  // This entity's PAST is currently being resolved in a parallel cosmos
                        //   its +1 causalchainlink entity should be in a forked state
        ghost,          // Exists vestigially from an intercession, must not be simulated, only 
                        //   played back, and can only be tangable to the intercesee
    };
    
    /*
        Future chainlink:
                                ╭─> merged : Interception is resolved and we can return to merged
        merged -> superposition ┤
                                ╰─> ghost  : Intercession triggered, we become ghost to resolve paradox

        Past chainlink:
        merged -> forking -> forked -> merging -> merged
                              │         │          ╰─> extracted from parallel
                              │         ╰─> being processed by parallel
                              ╰─> signals to parallel
    */

    // convenience function for capturing multiple stages of forking
    // Not including merging because that interaction should be captured & resolved during parallel simulation
    inline bool is_divergent(TimestreamState state)
    {
        return state == TimestreamState::forking || state == TimestreamState::forked;
    }
}

#endif // TIMESTREAM_STATE_H