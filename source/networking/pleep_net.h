#ifndef PLEEP_NET_H
#define PLEEP_NET_H

//#include "intercession_pch.h"

namespace pleep
{
namespace net
{
    // define some messagetype enum
    enum class PleepMessageType : uint32_t
    {
        update,
        intercession
    };

    void test_net();
}
}

#endif // PLEEP_NET_H