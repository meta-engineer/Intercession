#ifndef PLEEP_NET_H
#define PLEEP_NET_H

//#include "intercession_pch.h"
#include <cstdint>

#include "networking/net_i_server.h"
#include "networking/net_i_client.h"

namespace pleep
{
namespace net
{
    // define some messagetype enum
    enum class PleepMessageType : uint32_t
    {
        null,
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
            PLEEPLOG_TRACE("Found invalid connection to cleanup: " + std::to_string(remote->get_id()));

        }
        
        void on_message(std::shared_ptr<Connection<PleepMessageType>> remote, Message<PleepMessageType>& msg) override
        {
            UNREFERENCED_PARAMETER(remote);
            UNREFERENCED_PARAMETER(msg);

            switch (msg.header.id)
            {
            case net::PleepMessageType::update:
            {
                PLEEPLOG_TRACE("Bouncing update message");
                // just bounce back
                remote->send(msg);
            }
            break;
            case net::PleepMessageType::intercession:
            {
                PLEEPLOG_TRACE("Bouncing intercession message");
                // just bounce back
                remote->send(msg);
            }
            break;
            default:
            {
                PLEEPLOG_TRACE("Recieved unknown message");
            }
            break;
            }

        }
    };

    class PleepClient : public I_Client<PleepMessageType>
    {

    };

    void test_net();
}
}

#endif // PLEEP_NET_H