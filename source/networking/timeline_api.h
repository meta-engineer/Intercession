#ifndef TIMELINE_API_H
#define TIMELINE_API_H

//#include "intercession_pch.h"
#include <utility>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <string>

#include "logging/pleep_log.h"
#include "networking/ts_deque.h"
#include "networking/timeline_config.h"
#include "networking/entity_timestream_map.h"
#include "events/event_types.h"
#include "core/cosmos.h"

namespace pleep
{

    // We need a pointer to the parallel context
    // but the parallel context needs to have its own TimelineApi
    // so a foreward declaration is unfortunate, but needed
    class ParallelCosmosContext;


    // Provide all "calibratable" parameters for a single thread (and defaults)
    // Should also have a validator function to check values
    // Provide communication channels/addresses to access other timeslices
    // only uses EventMessage as transferable datatype
    class TimelineApi
    {
    public:
        // Should queue type contain the source id? Otherwise it has to be built into message manually
        // Hard-code message type to use EventId (to avoid template cascading)
        using Multiplex = std::unordered_map<TimesliceId, TsDeque<Message<EventId>>>;

        // Accept top level timeline config, and my individual timesliceId
        // accept Message Conduits shared for each individual api?
        TimelineApi(TimelineConfig cfg, TimesliceId id, 
                    std::shared_ptr<Multiplex> sharedMultiplex, 
                    std::shared_ptr<EntityTimestreamMap> pastTimestreams = nullptr,
                    std::shared_ptr<EntityTimestreamMap> futureTimestreams = nullptr,
                    std::shared_ptr<ParallelCosmosContext> parallelContext = nullptr);

        // get the unique timesliceId registered for this TimelineApi instance
        TimesliceId get_timeslice_id();
        // get total number of timeslices in the local network (ids SHOULD start from 0)
        size_t get_num_timeslices();
        uint16_t get_port();
        uint16_t get_timeslice_delay();
        uint16_t get_simulation_hz();

        // ***** Accessors for multiplex *****

        // Restrict access to outgoing queues to only be sendable
        bool send_message(TimesliceId id, const EventMessage& data);
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
        void push_past_timestream(Entity entity, const EventMessage& data);
        // give potenial Entities to pop from
        std::vector<Entity> get_entities_with_future_streams();
        // Restrict access for future timestreams to only be poppable
        bool pop_future_timestream(Entity entity, uint16_t coherency, EventMessage& dest);

        // copy pointer to sourceTimestreams, overwrites both m_future and m_past
        // (for parallel to use breakpoint functions)
        // pass nullptr to effectively unlink
        void link_timestreams(std::shared_ptr<EntityTimestreamMap> sourceTimestreams);
        // push to "past" at breakpoint
        void push_timestream_at_breakpoint(Entity entity, const EventMessage& data);
        // pop from "future" at breakpoint
        bool pop_timestream_at_breakpoint(Entity entity, uint16_t coherency, EventMessage& dest);

        // ***** Accessors for Parallel Context *****

        // notify parallel that forked entities are available to be resolved
        void parallel_notify_divergence();
        // deep copy cosmos, deep copy & link parallel to m_futureTimestreams
        void parallel_load_and_link(const std::shared_ptr<Cosmos> sourceCosmos);
        // sets parallel simulation target coherency.
        // must be greater than current or sets to current
        void parallel_retarget(uint16_t newTarget);
        // if simulation has reached target and stopped, restarts it
        // ideally set coherency target to be: current + 1 + delay to next (in frames)
        void parallel_start();
        // return the timeslice that parallel is currently simulating
        // returns NULL_TIMESLICEID if not running
        TimesliceId parallel_get_timeslice();
        // copy resolved entities from parallel into destination cosmos
        // (assumes extraction must involve decrementing)
        // if parallel is running, stops it and waits for it to finish
        bool parallel_extract(std::shared_ptr<Cosmos> dstCosmos);

    private:
        // data extracted from config, should not change after construction
        TimesliceId m_timesliceId;   // Store for Entity composition
        uint16_t m_delayToNextTimeslice;  // Coherency "units"/frames
        uint16_t m_simulationHz;
        
        // Direct message multiplex shared with all other timeslices
        std::shared_ptr<Multiplex> m_multiplex;

        // timestream queues shared with timeslices ahead and behind in the timeline
        // future-most timeslice will have no future, past-most timelice will have no past
        std::shared_ptr<EntityTimestreamMap> m_futureTimestreams;
        std::shared_ptr<EntityTimestreamMap> m_pastTimestreams;

        // Access to shared ParallelContext
        std::shared_ptr<ParallelCosmosContext> m_sharedParallel;

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
            // emplace with default constructor for TsDeque
            multiplex->operator[](i);
        }
        
        return multiplex;
    }
}

#endif // TIMELINE_API_H