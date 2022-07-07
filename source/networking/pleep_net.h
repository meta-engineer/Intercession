#ifndef PLEEP_NET_H
#define PLEEP_NET_H

//#include "intercession_pch.h"
#include <cstdint>

#include "networking/net_i_server.h"

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

    class PleepServer : public I_Server<PleepMessageType>
    {
    public:
        PleepServer(uint16_t port)
            : I_Server<PleepMessageType>(port)
        {
        }

    protected:
        bool on_remote_connect(std::shared_ptr<Connection<PleepMessageType>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            return true;
        }
        
        void on_remote_disconnect(std::shared_ptr<Connection<PleepMessageType>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);

        }
        
        void on_message(std::shared_ptr<Connection<PleepMessageType>> remote, Message<PleepMessageType>& msg) override
        {
            UNREFERENCED_PARAMETER(remote);
            UNREFERENCED_PARAMETER(msg);

        }
    };

    void test_net();
}
}

#endif // PLEEP_NET_H