#ifndef SERVER_NETWORK_DYNAMO_H
#define SERVER_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <memory>

#include "networking/i_network_dynamo.h"
// Access PleepNet ...
#include "server/intercession_server.h"

namespace pleep
{
    class ServerNetworkDynamo : public I_NetworkDynamo
    {
    public:
        // TODO: accept networking api (iocontext) from AppGateway
        ServerNetworkDynamo(EventBroker* sharedBroker);
        ~ServerNetworkDynamo();

        // TODO: what entities, if any, would be submitted each frame?
        //void submit() override;
        
        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // Networking relays

        // TEMP: build raw server instance
        std::unique_ptr<net::IntercessionServer> m_server;
    };
}

#endif // SERVER_NETWORK_DYNAMO_H