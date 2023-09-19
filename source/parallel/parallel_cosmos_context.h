#ifndef PARALLEL_COSMOS_CONTEXT_H
#define PARALLEL_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include "core/i_cosmos_context.h"

namespace pleep
{
    // async cosmos to run alternate/parallel timeline until a target coherency timepoint
    // and can then be extracted for entity data
    class ParallelCosmosContext : public I_CosmosContext
    {
    public:
        // initialize with empty cosmos
        ParallelCosmosContext();
        ~ParallelCosmosContext();

        // inject a message into the internal EventBroker of this cosmos
        void injectMessage(EventMessage msg);

        // receive a Cosmos pointer to deep copy for our parallel simulation
        void copyCosmos(std::shared_ptr<Cosmos> cosmos);

        // timepoint to stop simulating after reaching
        // should be called before run
        void set_coherency_target(uint16_t coherency);

        // call I_CosmosContext::run() asynchronously until coherency target
        //   automatically stops the context and the call exits

    protected:
        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;

        // timepoint when we should stop ourselves
        uint16_t m_coherencyTarget;
    };
}

#endif // PARALLEL_COSMOS_CONTEXT_H