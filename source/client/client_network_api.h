#ifndef CLIENT_NETWORK_API_H
#define CLIENT_NETWORK_API_H

//#include "intercession_pch.h"
#include "networking/net_i_client.h"
#include "events/event_types.h"

namespace pleep
{
    class ClientNetworkApi : public net::I_Client<EventId>
    {
    public:
        ClientNetworkApi()
            : net::I_Client<EventId>()
        {}

        // no callbacks required
    };
}

#endif // CLIENT_NETWORK_API_H