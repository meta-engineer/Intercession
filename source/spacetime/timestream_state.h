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
                ^               ╰─> ghost  : Intercession triggered, we become ghost to resolve paradox
                ╰─ Interception signal received (from past)

        Past chainlink:
        merged -> forking -> forked -> merged
                ^          ^         ^     
                │          │         ╰─ timestream being corrected by parallel
                │          ╰─ timeout elapsed, signals to parallel for resolution
                ╰─ Interception signal received (from present), waiting for timeout
    */

    // convenience function for capturing multiple stages of forking
    inline bool is_divergent(TimestreamState state)
    {
        return state == TimestreamState::forking || state == TimestreamState::forked;
    }
}

#endif // TIMESTREAM_STATE_H