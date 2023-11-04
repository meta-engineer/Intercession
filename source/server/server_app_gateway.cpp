#include "server_app_gateway.h"

#include "networking/timeline_api.h"
#include "events/event_types.h"

namespace pleep
{
    ServerAppGateway::ServerAppGateway(TimelineConfig cfg)
    {
        PLEEPLOG_TRACE("Start constructing server app gateway");
        // Persist any info from cfg?

        // Build apis

        // "local temporal network" api
        // shared message queues for all instances of the ltn api
        std::shared_ptr<TimelineApi::Multiplex> sharedMultiplex = generate_timeline_multiplex(cfg.numTimeslices);
        // Build contexts and pass apis
        PLEEPLOG_INFO("Constructing " + std::to_string(cfg.numTimeslices) + " timeslices");
        assert(m_contexts.empty());

        // maintain a pair of future & past timestream maps
        std::shared_ptr<EntityTimestreamMap> pastTimestreams = nullptr;
        std::shared_ptr<EntityTimestreamMap> futureTimestreams = nullptr;
        // same with parallel contexts
        std::shared_ptr<ParallelCosmosContext> pastParallelContext = nullptr;
        std::shared_ptr<ParallelCosmosContext> futureParallelContext = nullptr;

        // construct timeslices in reverse order so that origin/present (0) is created LAST
        //     remember TimesliceId is uint32, use underflow to stop loop
        for (TimesliceId i = cfg.numTimeslices - 1; i < cfg.numTimeslices; i--)
        {
            // swap future with past, construct new future
            pastTimestreams = futureTimestreams;
            pastParallelContext = futureParallelContext;
            if (i == 0)
            {
                // timeslice 0 will have no future
                futureTimestreams = nullptr;
                futureParallelContext = nullptr;
            }
            else
            {
                futureTimestreams = std::make_shared<EntityTimestreamMap>();
                futureParallelContext = std::make_shared<ParallelCosmosContext>();
            }

            PLEEPLOG_TRACE("Start constructing server context TimesliceId #" + std::to_string(i));

            // Inside each context the TimelineConfig information will only be accessible 
            //   through the TimelineApi (to branch based on specific timesliceId)
            std::unique_ptr<I_CosmosContext> ctx = std::make_unique<ServerCosmosContext>(
                TimelineApi(cfg, i, sharedMultiplex, pastTimestreams, futureTimestreams, pastParallelContext, futureParallelContext)
            );
            
            PLEEPLOG_TRACE("Done constructing server context TimesliceId #" + std::to_string(i));

            m_contexts.push_back(std::move(ctx));
        }
        PLEEPLOG_TRACE("Finished constructing " + std::to_string(m_contexts.size()) + " contexts");

        // TODO: initialize context state, and cosmos state of each (maybe through cascading?)
        // we could let the system run for total timeline duration as part of initializing?
        // otherwise state of timelineIds > 0 are... undefined...

        PLEEPLOG_TRACE("Done constructing server app gateway");
    }

    ServerAppGateway::~ServerAppGateway()
    {
        // call contexts to stop
        for (size_t i = 0; i < m_contexts.size(); i++)
        {
            m_contexts[i]->stop();
        }

        // wait for threads to join
        // (what if stop is unsuccessful they never finish execution?)
        for (size_t i = 0; i < m_contexts.size(); i++)
        {
            m_contexts[i]->join();
        }

        // clean apis
        // "local temporal network" api cleans with contexts
    }

    void ServerAppGateway::run()
    {
        if (m_contexts.empty())
        {
            PLEEPLOG_ERROR("AppGateway cannot be run when configured with 0 contexts");
            return;
        }
        // TODO: Validate contexts are ready to start

        PLEEPLOG_TRACE("App run begin");

        std::ostringstream thisThreadId; thisThreadId << std::this_thread::get_id();
        PLEEPLOG_INFO("AppGateway thread #" + thisThreadId.str());

        for (size_t i = 0; i < m_contexts.size(); i++)
        {
            // start each thread inside context's run loop
            PLEEPLOG_INFO("Starting context " + std::to_string(m_contexts.size() - 1 - i));
            m_contexts[i]->start();
        }
        PLEEPLOG_TRACE("Finished constructing " + std::to_string(m_contexts.size()) + " threads");

        // contexts are all now running
        // AppGateway needs to stay alive (not exit from run) until threads finish or are stopped
        // we can monitor context state & possibly modify them (recover from failures?)
        // (make sure we monitor them in a non-spinning way. CV notify when a thread completes? or just moderate sleep intervals)
        while(true)
        {
            // some work
            std::this_thread::sleep_for(std::chrono::duration<double>(1.0));

            // Do nothing for now
            break;
        }

        // Wait for all contexts to finish on their own
        for (size_t i = 0; i < m_contexts.size(); i++)
        {
            m_contexts[i]->join();
        }

        // TODO: read error state from stopped contexts and handle accordingly
        // for now just return

        PLEEPLOG_TRACE("App run end");
    }

}