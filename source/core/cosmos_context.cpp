#include "cosmos_context.h"

#include "logging/pleep_log.h"

// TODO This is temporary until proper cosmos staging is implemented
#include "staging/test_cosmos.h"

namespace pleep
{
    CosmosContext::CosmosContext()
        : m_currentCosmos(nullptr)
        , m_renderDynamo(nullptr)
        , m_controlDynamo(nullptr)
        , m_physicsDynamo(nullptr)
    {
        m_running = false;

        // build event listener
        m_eventBroker = new EventBroker();
        // setup context's "fallback" listeners
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, CosmosContext::_quit_handler));
    }
    
    CosmosContext::CosmosContext(GLFWwindow* windowApi)
        : CosmosContext()
    {
        // Broker is ready to be distributed

        // construct dynamos
        m_renderDynamo  = new RenderDynamo(m_eventBroker, windowApi);
        m_controlDynamo = new ControlDynamo(m_eventBroker, windowApi);
        m_physicsDynamo = new PhysicsDynamo(m_eventBroker);
        
        // build empty starting cosmos
        m_currentCosmos = new Cosmos();

        // populate starting cosmos
        // eventually we'll pass some config param here
        _build_cosmos();
    }
    
    CosmosContext::~CosmosContext()
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;

        delete m_physicsDynamo;
        delete m_controlDynamo;
        delete m_renderDynamo;

        delete m_eventBroker;
    }
    
    void CosmosContext::run()
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

            // ***** Cosmos Update *****
            // invokes all registered synchros to process their entities
            m_currentCosmos->update();

            // ***** Run fixed timesteps for dynamos *****
            // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
            //   know which dynamos to call fixed and which to call on frametime
            size_t stepsTaken = 0;
            m_timeRemaining += deltaTime;
            while (m_timeRemaining >= m_fixedTimeStep && stepsTaken <= m_maxSteps)
            {
                m_timeRemaining -= m_fixedTimeStep;
                stepsTaken++;

                m_physicsDynamo->run_relays(m_fixedTimeStep);
                m_controlDynamo->run_relays(m_fixedTimeStep);
            }

            // ***** Run "frame time" timsteps for dynamos *****
            m_renderDynamo->run_relays(deltaTime);
            
            // flush dynamos of all synchro submissions
            m_controlDynamo->reset_relays();
            m_physicsDynamo->reset_relays();
            m_renderDynamo->reset_relays();

            // ***** Post Processing *****
            // top ui layer in context for debug
            // TODO: abstract this to ui layer
            // TODO: implment some fetchable stats in Dynamos to show here
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            bool checkbox;
            // show ui window
            {
                static float f = 0.0f;
                static int counter = 0;

                // Create a window and append into it.
                ImGui::Begin("Context Debug");

                // Display some text (you can use a format strings too)
                ImGui::Text("This window runs above the cosmos");

                // Report Cosmos info
                std::string countString = "Entity Count: " + std::to_string(m_currentCosmos->get_entity_count());
                ImGui::Text(countString.c_str());

                // Edit bools storing our window open/close state
                ImGui::Checkbox("checkbox", &checkbox);

                // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                // Buttons return true when clicked (most widgets return true when edited/activated)
                if (ImGui::Button("Button"))
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }
            // Render out ui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            

            // ***** Finish Frame *****
            // Context gets last word on any final superceding actions
            // Maybe IDynamo should have a generic "flush" method
            //   and we invoke all dynamos (to avoid having to specify)
            
            // RenderDynamo calls swap buffers
            m_renderDynamo->flush_frame();
        }
        
        PLEEPLOG_TRACE("Exiting \"main loop\"");
        // any non-destructor cleanup?
    }
    
    void CosmosContext::stop()
    {
        m_running = false;
    }
    
    void CosmosContext::_build_cosmos()
    {
        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        build_test_cosmos(m_currentCosmos, m_eventBroker, m_renderDynamo, m_controlDynamo, m_physicsDynamo);
        // use imgui input in main loop do add more at runtime
    }
    
    void CosmosContext::_quit_handler(Event quitEvent)
    {
        // should only be subscribed to events given with type:
        // events::window::QUIT
        UNREFERENCED_PARAMETER(quitEvent);
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");

        this->stop();
    }

}