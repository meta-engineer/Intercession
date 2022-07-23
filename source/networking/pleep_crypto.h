#ifndef PLEEP_CRYPTO_H
#define PLEEP_CRYPTO_H

//#include "intercession_pch.h"
#include <cstdint>

namespace pleep
{
    // Written by Squirrel Eiserloh not pleep
    // https://www.youtube.com/watch?v=LWFzPP8ZbdU
    // though protected here in the pleep namespace
    uint32_t Squirrel3(int position, uint32_t seed = 0)
    {
        constexpr unsigned int BIT_NOISE1 = 0xB5297A4D;
        constexpr unsigned int BIT_NOISE2 = 0x68E31DA4;
        constexpr unsigned int BIT_NOISE3 = 0x1B56C4E9;
        constexpr unsigned int BIT_PLEEP  = 0x61336;

        unsigned int mangled = position;
        mangled *= BIT_NOISE1;
        mangled += seed ^ BIT_PLEEP;
        mangled ^= (mangled >> 8);
        mangled += BIT_NOISE2;
        mangled ^= (mangled << 8);
        mangled += BIT_NOISE3;
        mangled ^= (mangled >> 8);
        return mangled;
    }
}

#endif // PLEEP_CRYPTO_H