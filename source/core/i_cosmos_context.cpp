#include "i_cosmos_context.h"

#include "logging/pleep_log.h"

namespace pleep
{
    I_CosmosContext::I_CosmosContext()
        : m_currentCosmos(nullptr)
        , m_running(false)
    {
        // build event listener
        m_eventBroker = new EventBroker();
        // setup context's "fallback" listeners
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, I_CosmosContext::_quit_handler));
    }
    
    I_CosmosContext::~I_CosmosContext()
    {
        delete m_eventBroker;
    }
    
    void I_CosmosContext::run()
    {
        m_running = true;

        // main game loop
        PLEEPLOG_TRACE("Starting \"main loop\"");
        double lastTimeVal = glfwGetTime();
        double thisTimeVal;
        double deltaTime;

        while (m_running)
        {
            // ***** Init Frame *****
            // smarter way to get dt?
            thisTimeVal = glfwGetTime();
            deltaTime = thisTimeVal - lastTimeVal;
            lastTimeVal = thisTimeVal;
            
            // ***** Setup Frame *****
            this->_prime_frame();

            // ***** Run fixed timesteps for dynamos *****
            // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
            //   know which dynamos to call fixed and which to call on frametime
            size_t stepsTaken = 0;
            m_timeRemaining += deltaTime;
            while (m_timeRemaining >= m_fixedTimeStep && stepsTaken <= m_maxSteps)
            {
                m_timeRemaining -= m_fixedTimeStep;
                stepsTaken++;

                this->_on_fixed(m_fixedTimeStep);
            }

            // ***** Run "frame time" timsteps for dynamos *****
            this->_on_frame(deltaTime);

            // ***** Finish Frame *****
            // Context gets last word on any final superceding actions
            this->_clean_frame();
        }
        
        PLEEPLOG_TRACE("Exiting \"main loop\"");
        // any non-destructor cleanup?
    }
    
    void I_CosmosContext::stop()
    {
        m_running = false;
    }
    
    void I_CosmosContext::_quit_handler(Event quitEvent)
    {
        // should only be subscribed to events given with type:
        // events::window::QUIT
        UNREFERENCED_PARAMETER(quitEvent);
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");

        this->stop();
    }

}