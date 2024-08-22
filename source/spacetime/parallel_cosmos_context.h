#ifndef PARALLEL_COSMOS_CONTEXT_H
#define PARALLEL_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include <unordered_set>
#include "core/i_cosmos_context.h"
#include "networking/entity_timestream_map.h"
#include "networking/timeline_api.h"

namespace pleep
{
    /// TODO: We need to be able to detect when a timejump in local has NOT happened during parallel's simulation of the same moment...
    //   so that we can purge it from the jumped-to slice and continue simulating it
    // jump events are just generic creation events in the timestream...
    // ... maybe parallel can store an extra timestream buffer for special events to remember
    // if the buffer has an event this can mean another cycle is needed
    // when we see a "jump from" event in the local timeline, but the parallel version did not
    //   jump, we can insert a "no jump" event in the special buffer at the target coherency value
    // when we see a "jump to" event in the local timestream, we can look for a "no jump" event in
    //   the special buffer, then we know that the whole stream for that entity can be ignored.
    // we also have to know exactly when the jump is completed on the receiving timeslice
    // race condition could cause it to be delayed by a frame?
    // maybe receiving server should report a JUMP_RECEIPT with the exact coherency the jump was completed? That can be placed in timestream to mark successful jump

    // async, headless cosmos to run alternate/parallel timeline until a target coherency timepoint
    // and can then be extracted for entity data
    // shared between the whole timestream (via app gateway)
    // It is like another resource used by a Dynamo (Network Dynamo)
    class ParallelCosmosContext : public I_CosmosContext
    {
    public:
        // initialize with empty cosmos
        ParallelCosmosContext(TimelineApi localTimelineApi);
        ~ParallelCosmosContext();

        // These public methods are for functions at the meta-cosmos level (for interfacing between two cosmos')

        // notification from a timeslice that resolution is needed in their cosmos
        // Parallel Context will respond (if necessary) via timeline api so that the
        //   timeslice can respond when its cosmos state is stable
        void request_resolution(TimesliceId requesterId);

        // timepoint to stop simulating after reaching (should be called before run/start)
        void set_coherency_target(uint16_t coherency);

        // return currently simulation timeslice (in the recent future of this timeslice)
        TimesliceId get_current_timeslice();

        // receive a Cosmos pointer to deep copy for our parallel simulation
        // Don't copy entities with chainlink 0 because they won't exist in the future
        // returns false if cosmos already exists/running and cannot be overwritten
        bool load_and_link(const std::shared_ptr<Cosmos> sourceCosmos, 
                           const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams);

        // copy forked entities into destination cosmos
        // (decrementing chainlink before deserialization)
        // set entity timestream state to merged
        // before returning parallel will send followup message for re-initialization (except for timeslice 0)
        // returns false if cosmos was not stopped or synced
        // returns true if extraction was successful
        bool extract_entity_updates(std::shared_ptr<Cosmos> dstCosmos);

    protected:
        // event handlers
        void _divergence_handler(EventMessage divEvent);
        void _entity_removed_handler(EventMessage removalEvent);
        void _entity_created_handler(EventMessage creationEvent);
        void _worldline_shift_handler(EventMessage shiftEvent);
        void _timestream_interception_handler(EventMessage interceptionEvent);

        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;

        // past & future should not be able to access simultaneously, however future can access during internal thread running
        std::mutex m_cosmosMux;
        // control flags and signal setting during simulation (below)
        std::mutex m_runtimeMux;

        // state for orchestrating the content of the current cosmos
        enum State
        {
            idle,
            initializing,
            ready,       // ready for extraction or ready to start
            busy
        };
        ParallelCosmosContext::State m_currentState = State::idle;
        // timepoint when we should stop ourselves
        uint16_t    m_coherencyTarget;
        // flag indicates when present frontier is reached, start again from the past
        bool        m_isRecycleNeeded = false;
        // which timeslice cosmos are we paralleling (set after init, while simulating, and while waiting for extract)
        TimesliceId m_currentTimeslice = NULL_TIMESLICEID;

        // remember condemned entities during simulation for "extraction"
        std::unordered_set<Entity> m_condemnedEntities;

        // whitelist of entities whos state IS NOT to be overridden by the worldline reconstruction
        // when extracting to a timeline. (If this set is empty, extract all as normal)
        // (thus shifting to the previous worldline at the moment of extraction)
        std::unordered_set<Entity> m_readingSteinerEntities;

        // records the entities each entity has intercepted over the course of this history cycle
        // should reset at the end of each cycle
        std::unordered_map<Entity, std::queue<Entity>> m_interceptionHistory;
        
        // use to start new simulation cycle
        const size_t m_pastmostTimeslice; // default 0?
    };
}

#endif // PARALLEL_COSMOS_CONTEXT_H