#ifndef OSCILLATOR_COMPONENT_H
#define OSCILLATOR_COMPONENT_H

//#include "intercession_pch.h"

namespace pleep
{
    struct OscillatorComponent
    {
        float phase = 0.0f;

        float amplitude = 1.0f;
        float period = 2.0f;
    };
}

#endif // OSCILLATOR_COMPONENT_H