#ifndef PLEEP_NET_H
#define PLEEP_NET_H

//#include "intercession_pch.h"
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace pleep
{
namespace net
{
    // PleepNet that abstracts network access via asio
    void test_sockets();
}
}

#endif // PLEEP_NET_H