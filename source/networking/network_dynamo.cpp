#include "network_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    NetworkDynamo::NetworkDynamo(EventBroker* sharedBroker)
        : IDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Networking pipeline");
        // setup relays

        pleep::net::test_net();
        
        PLEEPLOG_TRACE("Done Networking pipeline setup");
    }

    NetworkDynamo::~NetworkDynamo()
    {
    }

    void NetworkDynamo::submit()
    {
        // dispatch to relays
    }

    void NetworkDynamo::run_relays(double deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);

    }

    void NetworkDynamo::reset_relays()
    {

    }
}