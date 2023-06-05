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

        // total timeline duration can be inferred as timesliceDelay * numTimeslices
        // (explicitly definine delay means duration is always cleanly divisible)
        // number of seconds between each timeslice
        int timesliceDelay = 5;
        // must not exceed TIMESLICEID_SIZE
        TimesliceId numTimeslices = 2;

        // Cosmos updates per second
        // simulation includes input polling, parsing incoming network messages, script updates, physics integration.collision
        double simulationHz = 90.0;
        // server updates don't need to happen every frame, client can dead reckon for a few
        // serializing the whole cosmos is costly so do it less frequently
        // (client still needs to emit their inputs every simulation update)
        double networkHz     = 30.0;
        // frame time runs asap after the above fixed timesteps (rendering/animation/ui)

    };
}

#endif // TIMELINE_CONFIG_H