#ifndef TIMELINE_API_H
#define TIMELINE_API_H

//#include "intercession_pch.h"
#include <utility>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <string>

#include "logging/pleep_log.h"
#include "networking/ts_queue.h"
#include "networking/timeline_config.h"
#include "networking/entity_timestream_map.h"
#include "events/event_types.h"
#include "spacetime/parallel_cosmos_context.h"

namespace pleep
{
    // Provide all "calibratable" parameters for a single thread (and defaults)
    // Should also have a validator function to check values
    // Provide communication channels/addresses to access other timeslices
    // only uses EventMessage as transferable datatype
    class TimelineApi
    {
    public:
        // Should queue type contain the source id? Otherwise it has to be built into message manually
        // Hard-code message type to use EventId (to avoid template cascading)
        using Multiplex = std::unordered_map<TimesliceId, TsQueue<Message<EventId>>>;

        // Accept top level timeline config, and my individual timesliceId
        // accept Message Conduits shared for each individual api?
        TimelineApi(TimelineConfig cfg, TimesliceId id, 
                    std::shared_ptr<Multiplex> sharedMultiplex, 
                    std::shared_ptr<EntityTimestreamMap> pastTimestreams = nullptr,
                    std::shared_ptr<EntityTimestreamMap> futureTimestreams = nullptr,
                    std::shared_ptr<ParallelCosmosContext> pastParallelContext = nullptr,
                    std::shared_ptr<ParallelCosmosContext> futureParallelContext = nullptr);

        // get the unique timesliceId registered for this TimelineApi instance
        TimesliceId get_timeslice_id();
        // get total number of timeslices in the local network (ids SHOULD start from 0)
        size_t get_num_timeslices();
        uint16_t get_port();
        uint16_t get_timeslice_delay();
        uint16_t get_simulation_hz();

        // ***** Accessors for multiplex *****

        // Restrict access to outgoing queues to only be sendable
        bool send_message(TimesliceId id, EventMessage& data);
        // check to prevent popping empty deque
        bool is_message_available();
        // Restrict access to incoming queue to only be poppable
        // returns false if nothing was available at time of call
        bool pop_message(EventMessage& dest);

        // ***** Accessors for Timestreams *****

        // detect if I have parent/child timeslices
        // ids may not necessarily be in order or strictly less than the config size
        bool has_future();
        bool has_past();

        // because event type is ambiguous, caller must specify entity regardless
        // data should have coherency set, but is not strictly enforced
        void push_past_timestream(Entity entity, Message<EventId>& data);
        // Removes all data in the future timestream for this data, BUT keeps its timestream indexed
        void clear_future_timestream(Entity entity);
        // give potenial Entities to pop from
        std::vector<Entity> get_entities_with_future_streams();
        // Restrict access for future timestreams to only be poppable
        bool pop_future_timestream(Entity entity, uint16_t coherency, EventMessage& dest);

        // ***** Parallel Context Access *****

        // deep copy cosmos, deep copy future timstream
        // set coherency target to be: current + 1 + delay to next (in frames)
        // resolutionCandidates: Entities which are forked and should be extracted in future
        // nonCandidates: Entities which are forked, but did not meet other resolution criteria so they should be considered as TimestreamState::merged
        void future_parallel_init_and_start(
            const std::shared_ptr<Cosmos> sourceCosmos,
            const std::unordered_set<Entity>& resolutionCandidates,
            const std::unordered_set<Entity>& nonCandidates
        );
        // parallel simulation is finished
        bool future_parallel_is_closed();

        // use to end simulation and signal entities have been resolved
        void past_parallel_close();
        // parallel simulation is finished
        bool past_parallel_is_closed();
        // sets parallel simulation target coherency.
        // must be greater than current or sets to current and stops
        // if simulation has reached target and stopped, restarts it
        void past_parallel_set_target_coherency(uint16_t newTarget);
        uint16_t past_parallel_get_current_coherency();
        // return copy of forked entity list
        const std::vector<Entity> past_parallel_get_forked_entities();
        // Produce an Entity update message for the given Entity
        bool past_parallel_extract_entity(Entity e, EventMessage& dst);

    private:
        TimesliceId m_timesliceId;   // Store for Entity composition
        uint16_t m_delayToNextTimeslice;  // Coherency "units"/frames
        uint16_t m_simulationHz;
        
        // Direct message multiplex shared with all other timeslices
        std::shared_ptr<Multiplex> m_multiplex;

        // timestream queues shared with timeslices ahead and behind in the timeline
        // future-most timeslice will have no future, past-most timelice will have no past
        std::shared_ptr<EntityTimestreamMap> m_futureTimestreams;
        std::shared_ptr<EntityTimestreamMap> m_pastTimestreams;

        // ParallelContext we setup for future
        std::shared_ptr<ParallelCosmosContext> m_futureParallelContext;
        // ParallelContext setup by past for us
        std::shared_ptr<ParallelCosmosContext> m_pastParallelContext;

        uint16_t m_port;
    };

    // return a multiplex map with empty queues for ids 0 to (numUsers - 1)
    // this should be passed to each TimelineApi, along with each unique id
    inline std::shared_ptr<TimelineApi::Multiplex> generate_timeline_multiplex(TimesliceId numUsers)
    {
        assert(numUsers < TIMESLICEID_SIZE);
        
        std::shared_ptr<TimelineApi::Multiplex> multiplex = std::make_shared<TimelineApi::Multiplex>();
        for (TimesliceId i = 0; i < numUsers; i++)
        {
            // emplace with default constructor for TsQueue
            multiplex->operator[](i);
        }
        
        return multiplex;
    }
}

#endif // TIMELINE_API_H