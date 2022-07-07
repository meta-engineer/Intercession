#ifndef SERVER_NETWORK_DYNAMO_H
#define SERVER_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <memory>

// Access PleepNet ...
#include "networking/pleep_net.h"

#include "core/a_dynamo.h"

namespace pleep
{
    class ServerNetworkDynamo : public A_Dynamo
    {
    public:
        // TODO: accept networking api (iocontext) from AppGateway
        ServerNetworkDynamo(EventBroker* sharedBroker);
        ~ServerNetworkDynamo();

        
        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // Networking relays

        // TEMP: build raw PleepServer
        std::unique_ptr<net::PleepServer> m_server;
    };
}

#endif // SERVER_NETWORK_DYNAMO_H