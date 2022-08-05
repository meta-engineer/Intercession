#ifndef CLIENT_NETWORK_DYNAMO_H
#define CLIENT_NETWORK_DYNAMO_H

//#include "intercession_pch.h"

#include "networking/i_network_dynamo.h"

// Access PleepNet ...
#include "networking/pleep_net.h"

namespace pleep
{
    class ClientNetworkDynamo : public I_NetworkDynamo
    {
    public:
        // TODO: accept networking api (iocontext) from AppGateway
        ClientNetworkDynamo(EventBroker* sharedBroker);
        ~ClientNetworkDynamo();

        // TODO: what entities, if any, would be submitted each frame?
        // should they be shared in I_NetworkDynamo?
        //void submit() override;
        
        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // Networking relays

        // TEMP: build raw PleepClient
        std::unique_ptr<net::PleepClient> m_client;
    };
}

#endif // CLIENT_NETWORK_DYNAMO_H