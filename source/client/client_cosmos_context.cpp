#include "client_cosmos_context.h"

// TODO: This is temporary until proper cosmos staging is implemented
#include "staging/test_cosmos.h"

namespace pleep
{
    ClientCosmosContext::ClientCosmosContext(GLFWwindow* windowApi)
        : I_CosmosContext()
        , m_renderDynamo(nullptr)
        , m_controlDynamo(nullptr)
        , m_physicsDynamo(nullptr)
        , m_networkDynamo(nullptr)
    {
        // I_CosmosContext() has setup broker
        
        // construct dynamos
        m_renderDynamo  = new RenderDynamo(m_eventBroker, windowApi);
        m_controlDynamo = new ControlDynamo(m_eventBroker, windowApi);
        m_physicsDynamo = new PhysicsDynamo(m_eventBroker);
        m_networkDynamo = new NetworkDynamo(m_eventBroker);
        
        // build empty starting cosmos
        m_currentCosmos = new Cosmos();
        
        // populate starting cosmos
        // TODO: use network dynamo to get cosmos from server
        this->_build_cosmos();
    }
    
    ClientCosmosContext::~ClientCosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;

        delete m_networkDynamo;
        delete m_physicsDynamo;
        delete m_controlDynamo;
        delete m_renderDynamo;
    }
    
    void ClientCosmosContext::_prime_frame() 
    {
        // ***** Cosmos Update *****
        // invokes all registered synchros to process their entities
        // this fills the dynamo relays with packets which should be cleared in _clean_frame()
        m_currentCosmos->update();
    }
    
    void ClientCosmosContext::_on_fixed(double fixedTime)
    {
        // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
        //   know which dynamos to call fixed and which to call on frametime
        m_networkDynamo->run_relays(fixedTime);
        m_physicsDynamo->run_relays(fixedTime);
        m_controlDynamo->run_relays(fixedTime);
    }
    
    void ClientCosmosContext::_on_frame(double deltaTime) 
    {
        m_renderDynamo->run_relays(deltaTime);

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
            ImGui::Begin("Client Context Debug");

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
    }
    
    void ClientCosmosContext::_clean_frame() 
    {
        // TODO: Maybe A_Dynamo should have a generic "flush" method
        //   and we invoke all dynamos (to avoid having to specify)
        
        // flush dynamos of all synchro submissions
        m_networkDynamo->reset_relays();
        m_physicsDynamo->reset_relays();
        m_controlDynamo->reset_relays();
        m_renderDynamo->reset_relays();
        
        // RenderDynamo calls swap buffers
        m_renderDynamo->flush_frame();
    }

    
    void ClientCosmosContext::_build_cosmos()
    {
        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        build_test_cosmos(m_currentCosmos, m_eventBroker, m_renderDynamo, m_controlDynamo, m_physicsDynamo);
        // use imgui input in main loop do add more at runtime
    }
}