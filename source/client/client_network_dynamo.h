#ifndef CLIENT_NETWORK_DYNAMO_H
#define CLIENT_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <unordered_set>

#include "networking/i_network_dynamo.h"
#include "core/cosmos.h"

// Access network interface
#include "client/intercession_client.h"

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
        // should they be shared in I_NetworkDynamo?
        void submit(CosmosAccessPacket data) override;

    private:
        void _entity_modified_handler(EventMessage entityEvent);

        // Networking relays

        // TEMP: build raw client instance
        std::unique_ptr<net::IntercessionClient> m_client;

        // TODO: Should this be held in a specific relay?
        Cosmos* m_workingCosmos = nullptr;
        std::unordered_set<Entity> m_entitiesToReport;
    };
}

#endif // CLIENT_NETWORK_DYNAMO_H