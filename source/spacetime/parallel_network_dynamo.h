#ifndef PARALLEL_NETWORK_DYNAMO_H
#define PARALLEL_NETWORK_DYNAMO_H

//#include "intercession_pch.h"
#include <memory>
#include <unordered_set>

#include "networking/i_network_dynamo.h"
#include "networking/timeline_api.h"
#include "spacetime/timejump_conditions.h"

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
        
        // Overwrite current timelineApi timestreams with the sourceTimestreams and set breakpoints
        void link_timestreams(std::shared_ptr<EntityTimestreamMap> sourceTimestreams) override;

    private:
        // event handlers
        void _entity_created_handler(EventMessage creationEvent);
        void _entity_removed_handler(EventMessage removalEvent);
        void _timestream_interception_handler(EventMessage interceptionEvent);
        void _parallel_init_handler(EventMessage initEvent);
        void _parallel_finished_handler(EventMessage finishedEvent);
        void _jump_request_handler(EventMessage jumpEvent);

        // TimelineApi (generated for us by AppGateway) to communicate with servers
        TimelineApi m_timelineApi;
        
        std::weak_ptr<Cosmos> m_workingCosmos;

        // cache of departure conditions to match between timestream (during network dynamo) and cosmos (during behaviours dynamo)
        // non-matching departures are determined to be divergent and promoted to m_divergentJumpRequests
        // cleared after each frame
        std::unordered_map<Entity, TimejumpConditions> m_jumpConditions;

        // cache of tripId, and entity data at a departure which diverged locally from the timestream
        // if we reach an arrival with a matching tripId in timestrean, override with the cached data
        std::unordered_map<uint32_t, EventMessage> m_divergentJumpRequests;
    };
}

#endif // PARALLEL_NETWORK_DYNAMO_H