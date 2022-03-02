#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

//#include "intercession_pch.h"
#include <bitset>

namespace pleep
{
    // this is an abstraction of all information needed from an input device
    // it should be specific to the type of control dynamo using it
    // Also should probably be given a specific prefix (could subclass)
    class InputBuffer
    {
        // bitsets are convenient
        // actions should be ambiguous of source device
        std::bitset<8> buttons;
    };
}

#endif // INPUT_BUFFER_H