#ifndef PARALLEL_COSMOS_CONTEXT_H
#define PARALLEL_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include <unordered_set>
#include "core/i_cosmos_context.h"
#include "networking/entity_timestream_map.h"

namespace pleep
{
    // async, headless cosmos to run alternate/parallel timeline until a target coherency timepoint
    // and can then be extracted for entity data
    // shared between past & future timeslices to setup, run, and extract updates from cosmos
    class ParallelCosmosContext : public I_CosmosContext
    {
    public:
        // initialize with empty cosmos
        ParallelCosmosContext();
        ~ParallelCosmosContext();

        // receive a Cosmos pointer to deep copy for our parallel simulation
        void init_cosmos(const std::shared_ptr<Cosmos> sourceCosmos, const std::shared_ptr<EntityTimestreamMap> sourceFutureTimestreams, const std::unordered_set<Entity>& resolutionCandidates, const std::unordered_set<Entity>& nonCandidates);

        // timepoint to stop simulating after reaching
        // should be called before run
        void set_coherency_target(uint16_t coherency);

        // returns coherency simulation has reached
        // if simulation is not joined return 0 (too volatile to be useful)
        uint16_t get_current_coherency();

        // atomic copy of entties which have forked since start of cosmos
        const std::vector<Entity> get_forked_entities();

        // atomic copy entity data into new message
        bool extract_entity(Entity e, EventMessage& dst);

    protected:
        // event handlers
        void _timestream_interception_handler(EventMessage interceptionEvent);

        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;

        // timepoint when we should stop ourselves
        uint16_t m_coherencyTarget;

        // past & future should not be able to access simultaneously, however future can access during internal thread running
        std::mutex m_accessMux;
        
        // Context owns simulation thread only for running async
        std::thread m_futureParallelThread;

        // track all entities which enter forked state during a parallel simulation
        std::vector<Entity> m_forkedEntities;
    };
}

#endif // PARALLEL_COSMOS_CONTEXT_H