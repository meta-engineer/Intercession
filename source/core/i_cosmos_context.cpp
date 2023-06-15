#include "i_cosmos_context.h"

#include "logging/pleep_log.h"

namespace pleep
{
    I_CosmosContext::I_CosmosContext()
    {
        // build event listener
        m_eventBroker = new EventBroker();
        // setup context's "fallback" listeners
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, I_CosmosContext::_quit_handler));
    }
    
    I_CosmosContext::~I_CosmosContext()
    {
        PLEEPLOG_WARN("Deleting event broker!");
        // Careful, dynamos will be deleted after this, so if they try to reference event broker, they will segfault. This should be put in a shared pointer and declared after dynamos
        delete m_eventBroker;
    }
    
    void I_CosmosContext::run()
    {
        m_running = true;

        // main game loop
        PLEEPLOG_TRACE("Starting \"frame loop\"");
        std::chrono::system_clock::time_point lastTimeVal = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point thisTimeVal;
        std::chrono::duration<double> deltaTime;

        try
        {
            while (m_running)
            {
                // ***** Init Frame *****
                // smarter way to get dt? duration.count() is in seconds?
                thisTimeVal = std::chrono::system_clock::now();
                deltaTime = thisTimeVal - lastTimeVal;
                lastTimeVal = thisTimeVal;

                // ***** Setup Frame *****
                this->_prime_frame();

                // ***** Run fixed timestep(s) *****
                size_t stepsTaken = 0;
                m_timeRemaining += deltaTime;
                while (m_timeRemaining >= m_fixedTimeStep && stepsTaken <= m_maxSteps)
                {
                    m_timeRemaining -= m_fixedTimeStep;
                    stepsTaken++;

                    this->_on_fixed(m_fixedTimeStep.count());
                    if (m_currentCosmos) m_currentCosmos->increment_coherency();
                }

                // ***** Run "frame time" timestep *****
                this->_on_frame(deltaTime.count());

                // ***** Finish Frame *****
                // Context gets last word on any final superceding actions
                this->_clean_frame();

                // TODO: let Cosmos make any volitile changes now that entity references are cleared
                // e.g. cleanup all entities signalled to be deleted during frame
                // who should listen for delete requests? me or Cosmos?
                //m_currentCosmos->flush_entity_changes?();
            }
        }
        catch (const std::exception& expt)
        {
            UNREFERENCED_PARAMETER(expt);
            PLEEPLOG_ERROR("The following uncaught exception occurred during CosmosContext::run(): ");
            PLEEPLOG_ERROR(expt.what());

            // TODO: store exception or some other error state for AppGateway to read after thread finishes
        }
        
        PLEEPLOG_TRACE("Exiting \"frame loop\"");
        // any non-destructor cleanup?
    }
    
    void I_CosmosContext::stop()
    {
        m_running = false;
    }
    
    void I_CosmosContext::_quit_handler(EventMessage quitEvent)
    {
        // should only be subscribed to events given with type:
        // events::window::QUIT
        UNREFERENCED_PARAMETER(quitEvent);
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");

        this->stop();
    }

}