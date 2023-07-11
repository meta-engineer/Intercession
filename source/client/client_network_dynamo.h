#ifndef CLIENT_NETWORK_DYNAMO_H
#define CLIENT_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <unordered_set>

#include "networking/i_network_dynamo.h"
#include "core/cosmos.h"

// Access network interface
#include "client/client_network_api.h"

namespace pleep
{
    class ClientNetworkDynamo : public I_NetworkDynamo
    {
    public:
        // Broker stored by I_NetworkDynamo -> A_Dynamo as m_sharedBroker
        ClientNetworkDynamo(EventBroker* sharedBroker);
        ~ClientNetworkDynamo();

        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;
        
        // TODO: what entities, if any, would be submitted each frame?
        // should they be shared in I_NetworkDynamo? or specified for my relays
        void submit(CosmosAccessPacket data) override;

        size_t get_num_connections() override;

        void restart_connection(const std::string& address, uint16_t port) override;

    private:
        // event handlers
        void _jump_request_handler(EventMessage jumpMsg);

        // Networking relays

        // Client instance to communicate with server
        ClientNetworkApi m_networkApi;

        // TODO: Move this to be held in a specific relay
        std::weak_ptr<Cosmos> m_workingCosmos;

        // code for validating timeslice jumps
        uint32_t m_transferCode = 0;
    };
}

#endif // CLIENT_NETWORK_DYNAMO_H