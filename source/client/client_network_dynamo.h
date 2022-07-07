#ifndef CLIENT_NETWORK_DYNAMO_H
#define CLIENT_NETWORK_DYNAMO_H

//#include "intercession_pch.h"

#include "core/a_dynamo.h"

// Access PleepNet ...
#include "networking/pleep_net.h"

namespace pleep
{
    class ClientNetworkDynamo : public A_Dynamo
    {
    public:
        // TODO: accept networking api (iocontext) from AppGateway
        ClientNetworkDynamo(EventBroker* sharedBroker);
        ~ClientNetworkDynamo();

        
        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // Networking relays

    };
}

#endif // CLIENT_NETWORK_DYNAMO_H