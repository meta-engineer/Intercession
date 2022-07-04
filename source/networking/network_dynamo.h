#ifndef NETWORK_DYNAMO_H
#define NETWORK_DYNAMO_H

//#include "intercession_pch.h"

#include <vector>
#include <memory>

// Access PleepNet ...
#include "networking/pleep_net.h"

#include "core/i_dynamo.h"
#include "events/event_broker.h"

namespace pleep
{
    class NetworkDynamo : public IDynamo
    {
    public:
        // TODO: accept networking api (iocontext) from AppGateway
        NetworkDynamo(EventBroker* sharedBroker);
        ~NetworkDynamo();

        // pass in entities which are updated over the network
        void submit();

        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // Networking relays

    };
}

#endif // NETWORK_DYNAMO_H