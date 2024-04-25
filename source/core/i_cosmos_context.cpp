#include "i_cosmos_context.h"

#include "logging/pleep_log.h"

namespace pleep
{
    I_CosmosContext::I_CosmosContext()
    {
        // build event listener
        m_eventBroker = std::make_shared<EventBroker>();
        // setup context's "fallback" listeners
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, I_CosmosContext::_quit_handler));
    }
    
    I_CosmosContext::~I_CosmosContext()
    {
    }
    
    void I_CosmosContext::run()
    {
        // incase another thread is already running (make this atomic?)
        if (m_isRunning) return;

        m_isRunning = true;

        // main game loop
        PLEEPLOG_TRACE("Starting \"frame loop\"");
        std::chrono::system_clock::time_point lastTimeVal = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point thisTimeVal;
        std::chrono::duration<double> deltaTime;

        try
        {
            while (m_isRunning)
            {
                // ***** Init Frame *****
                // smarter way to get dt? duration.count() is in seconds?
                thisTimeVal = std::chrono::system_clock::now();
                deltaTime = thisTimeVal - lastTimeVal;

                /// If not enough time has passed for either fixed or frame update
                /// then don't waist effort priming
                if (m_fixedTimeRemaining + deltaTime < m_fixedTimeStep 
                    && m_frameTimeRemaining + deltaTime < m_minFrameTimestep)
                {
                    /// TODO: stop this busy waiting? Is sleeping a good idea on a non-RTOS?
                    continue;
                }

                m_fixedTimeRemaining += deltaTime;
                m_frameTimeRemaining += deltaTime;
                lastTimeVal = thisTimeVal;

                // ***** Setup Frame *****
                this->_prime_frame();

                // ***** Run fixed timestep(s) *****
                if (m_fixedTimeRemaining >= m_fixedTimeStep)
                {
                    m_fixedTimeRemaining -= m_fixedTimeStep;
                    this->_on_fixed(m_fixedTimeStep.count());
                    // only increment coherency when simulation steps forward
                    
                    if (m_currentCosmos) m_currentCosmos->increment_coherency();
                }
                if (m_fixedTimeRemaining >= m_fixedTimeStep * 3.0)
                {
                    PLEEPLOG_WARN("Frame loop falling behind by " + std::to_string(m_fixedTimeRemaining.count() / m_fixedTimeStep.count()) + " frames.");
                }

                // ***** Run "frame time" timestep *****
                if (m_frameTimeRemaining >= m_minFrameTimestep)
                {
                    this->_on_frame(deltaTime.count());
                    using namespace std::chrono_literals;
                    m_frameTimeRemaining = 0s;
                }

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
            PLEEPLOG_ERROR("The following uncaught exception occurred during CosmosContext::run(): " + std::string(expt.what()));

            // TODO: store exception or some other error state for AppGateway to read after thread finishes
        }

        m_isRunning = false;
        PLEEPLOG_TRACE("Exiting \"frame loop\"");
        // any non-destructor cleanup?
    }
    
    void I_CosmosContext::stop()
    {
        m_isRunning = false;
    }
    
    bool I_CosmosContext::is_running() const
    {
        return m_isRunning;
    }
    
    void I_CosmosContext::start()
    {
        if (is_running()) return;
        // thread is not running, but may not have joined
        join();
        
        m_cosmosThread = std::thread(&I_CosmosContext::run, this);
        
        std::ostringstream thrdId; thrdId << m_cosmosThread.get_id();
        PLEEPLOG_INFO("Constructed thread id " + thrdId.str());
    }

    bool I_CosmosContext::join()
    {
        std::ostringstream threadId; threadId << m_cosmosThread.get_id();
        if (m_cosmosThread.joinable())
        {
            m_cosmosThread.join();
            PLEEPLOG_INFO("Thread " + threadId.str() + " joined.");
            return true;
        }
        else
        {
            PLEEPLOG_WARN("Thread " + threadId.str() + " was not joinable, skipping...");
            return false;
        }
    }

    bool I_CosmosContext::joinable()
    {
        return m_cosmosThread.joinable();
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