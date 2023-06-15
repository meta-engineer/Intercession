#ifndef SERVER_NETWORK_DYNAMO_H
#define SERVER_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <memory>

#include "networking/i_network_dynamo.h"
#include "server/server_network_api.h"
#include "networking/timeline_api.h"

namespace pleep
{
    // Driver for all networking functions
    // receives a TimelineApi passed from Appgateway which provides communication to
    // Network dynamos in other concurrent timeslices
    // It also constructs a server using the TimelineApi configuration
    // Network dynamo should handle networking typed events and dispatch them to
    // either the api (to other servers) or the server interface (to clients)
    // incoming messages on the server or api should be submitted to specific relays 
    // (or emitted as events to the rest of the context)
    // We handle all message by polling the networkApi.
    // But the networkApi handles all connected/validated/disconnected events asynchronously!
    class ServerNetworkDynamo : public I_NetworkDynamo
    {
    public:
        ServerNetworkDynamo(EventBroker* sharedBroker, TimelineApi localTimelineApi);
        ~ServerNetworkDynamo();

        // process network packet queues
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

        // Cosmos should be used to fetch all temporal entities?
        void submit(CosmosAccessPacket data) override;

        TimesliceId get_timeslice_id() override;

        size_t get_num_connections() override;

    private:
        // event handlers
        void _entity_created_handler(EventMessage entityEvent);
        void _entity_modified_handler(EventMessage entityEvent);
        void _entity_removed_handler(EventMessage entityEvent);
        
        // Networking relays

        // TimelineApi (generated for us by AppGateway) to communicate with other servers
        TimelineApi m_timelineApi;

        // Server instance to communicate with clients (we create from m_timelineApi config)
        ServerNetworkApi m_networkApi;

        std::weak_ptr<Cosmos> m_workingCosmos;
    };
}

#endif // SERVER_NETWORK_DYNAMO_H