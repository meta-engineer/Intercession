#ifndef RAW_INPUT_BUFFER_H
#define RAW_INPUT_BUFFER_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Raw input needs to capture:
    // analog (floating point) inputs on multiple dimensions (mouse/stick)
    // digital (binary) inputs (including rising and falling edges)

    // digital inputs could be mouse button or key value (up to ~350)
    // (values 0-31 are not used by keys, so we could use them for mouse)
    // we want the same information from both (held, released, rising, falling)
    // callbacks only give rising & falling edges
    // so we'll have to track two bools for each: state and edge

    struct RawInputBuffer
    {
        static const size_t numTwoDimAnalogs = 3;
        // really an array of vec2
        double twoDimAnalog[numTwoDimAnalogs][2];

        // used to transfer world-space information from client to server
        static const size_t numThreeDimAnalogs = 1;
        double threeDimAnalog[numThreeDimAnalogs][3];

        // values less than 256 are ascii equivalent
        // values over 256 are non printable keys as defined by glfw
        // https://www.glfw.org/docs/3.3/group__kdeys.html
        static const size_t numDigitals = 350;
        std::bitset<numDigitals> digitalState;
        // true -> current digitalState was risen/fallen to on this frame
        std::bitset<numDigitals> digitalEdge;

        // mods are in order: short, control, alt, super, capslock, numlock
        static const size_t numMods = 6;
        // do we need to track edge for these as well?
        std::bitset<numMods> mod;

        inline void setTwoDimAnalog(size_t index, double x, double y)
        {
            if (index > numTwoDimAnalogs)
            {
                PLEEPLOG_WARN("Cannot set twoDimAnalog value " + std::to_string(index) + " which exceeds twoDimAnalog array size");
                return;
            }
            twoDimAnalog[index][0] = x;
            twoDimAnalog[index][1] = y;
        }

        // automatically set digitalEdge
        inline void setDigital(size_t val, bool state)
        {
            if (val > numDigitals)
            {
                PLEEPLOG_WARN("Cannot set digital value " + std::to_string(val) + " which exceeds digital bitset size");
                return;
            }
            digitalState.set(val, state);
            digitalEdge.set(val, true);
        }

        // convert bitfield to bitset
        inline void convertMods(int modfield)
        {
            // can throw exception if mod value exceeds bit capacity
            mod = std::bitset<RawInputBuffer::numMods>(modfield);
        }

        // clear values which should be reset even if they don't receive a callback
        // use max as NULL (hopefully no mouse/controller can create this)
        inline void flush()
        {
            for (int i = 0; i < numTwoDimAnalogs; i++)
            {
                twoDimAnalog[i][0] = std::numeric_limits<double>::max();
                twoDimAnalog[i][1] = std::numeric_limits<double>::max();
            }
            for (int i = 0; i < numThreeDimAnalogs; i++)
            {
                threeDimAnalog[i][0] = std::numeric_limits<double>::max();
                threeDimAnalog[i][1] = std::numeric_limits<double>::max();
            }
            digitalEdge.reset();
        }
        
        inline void clear()
        {
            flush();
            digitalState.reset();
            mod.reset();
        }
    };
}

#endif // RAW_INPUT_BUFFER_H