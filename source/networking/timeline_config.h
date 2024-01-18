#ifndef TIMELINE_CONFIG_H
#define TIMELINE_CONFIG_H

//#include "intercession_pch.h"
#include <string>
#include "ecs/ecs_types.h"

namespace pleep
{
    // Describe all "calibratable" parameters for an app (and defaults)
    // Should also have a validator function to check values
    struct TimelineConfig
    {
        // Address is shared for all timeslices
        std::string timelineAddress = "127.0.0.1";
        // Port number for timeslice 0. Further timeslices will have to offset by their id
        // Therefore: highest port == presentPort + numTimeslices - 1
        // EX: origin = 61336 -> 2nd = 61337, 3rd = 61338
        // (they may also need a clause to increment & try again if there is a collision)
        uint16_t presentPort = 61336; // "PLEEP"

        // Cosmos updates per second
        // simulation includes input polling, parsing incoming network messages, behavior updates, physics integration.collision
        uint16_t simulationHz = 60;
        // server updates don't need to happen every frame, client can dead reckon for a few
        // serializing the whole cosmos is costly so do it less frequently
        // (client upstream changes would still need to happen every simulation update)
        //double networkHz    = 30.0;
        // renderHz is as-fast-as-possible after the above fixed timesteps (rendering/animation/ui)

        // number of frames between each timeslice
        // total timeline duration can be inferred as timesliceDelay * numTimeslices
        // (explicitly define delay means total duration is always cleanly divisible)
        // total delay in seconds is timesliceDelay / simulationHz
        uint16_t timesliceDelay = 10 * simulationHz;
        // must not exceed TIMESLICEID_SIZE
        TimesliceId numTimeslices = 2;
    };
}

#endif // TIMELINE_CONFIG_H