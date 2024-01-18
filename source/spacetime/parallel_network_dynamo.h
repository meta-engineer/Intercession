#ifndef PARALLEL_NETWORK_DYNAMO_H
#define PARALLEL_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <memory>
#include <unordered_set>

#include "networking/i_network_dynamo.h"
#include "networking/timeline_api.h"

namespace pleep
{
    // faux network dynamo to allow parallel cosmos to digest timestreams
    // and interface with other timeslices for extraction/initialization
    class ParallelNetworkDynamo : public I_NetworkDynamo
    {
    public:
        ParallelNetworkDynamo(std::shared_ptr<EventBroker> sharedBroker, TimelineApi localTimelineApi);
        ~ParallelNetworkDynamo();

        // no network packets, but process messages from slices
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

        // Cosmos should be used to fetch all entities to broadcast?
        void submit(CosmosAccessPacket data) override;

    private:
        // event handlers
        void _timestream_interception_handler(EventMessage interceptionEvent);
        void _parallel_init_handler(EventMessage initEvent);
        void _parallel_finished_handler(EventMessage finishedEvent);

        // TimelineApi (generated for us by AppGateway) to communicate with servers
        TimelineApi m_timelineApi;
        
        std::weak_ptr<Cosmos> m_workingCosmos;
    };
}

#endif // PARALLEL_NETWORK_DYNAMO_H