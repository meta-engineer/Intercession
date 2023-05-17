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
        // total timeline duration can be inferred as timesliceDelay * numTimeslices
        // (explicitly definine delay means duration is always cleanly divisible)

        // number of seconds between each timeslice
        int timesliceDelay = 5;
        // must not exceed TIMESLICEID_SIZE
        TimesliceId numTimeslices = 2;

        // updates per second
        double simulationHz = 200;
        double networkHz    = 30;

        // IP address for all timeslices is implicit to the machine
        
        // Port number for timeslice 0. Further timeslices will have to offset by their id
        // Therefore: highest port == originPort + numTimeslices - 1
        // EX: origin = 61336 -> 2nd = 61337, 3rd = 61338
        // (they may also need a clause to increment & try again if there is a collision)
        uint16_t originPort = 61336; // "PLEEP" 
    };
}

#endif // TIMELINE_CONFIG_H