#include "cosmos_context.h"

#include "logging/pleep_log.h"

namespace pleep
{
    CosmosContext::CosmosContext() 
        : m_running(false)
    {
        
    }
    
    CosmosContext::CosmosContext(GLFWwindow* windowApi)
    {
        // construct dynamos
        m_renderDynamo = new RenderDynamo(windowApi);
        m_controlDynamo  = new ControlDynamo(windowApi);

        // init config and cross-config dynamos before passing to Cosmos
        m_controlDynamo->attach_render_dynamo(m_renderDynamo);
        
        // build init cosmos
        // cosmos will build synchros and link with dynamos
        m_currentCosmos = new Cosmos(m_renderDynamo, m_controlDynamo);
        
        // until we can load from file we can manually call methods to build entities in its ecs
        // use imgui input in main loop do add more at runtime
    }
    
    CosmosContext::~CosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;

        m_controlDynamo->attach_render_dynamo(nullptr);
        delete m_controlDynamo;
        delete m_renderDynamo;
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

            // ***** Cosmos Update *****
            m_currentCosmos->update(deltaTime);

            // check Cosmos for some update/end state and handle appropriately
            // is polling every frame ideal?
            switch (m_currentCosmos->get_status())
            {
                case (Cosmos::Status::OK):
                    break;
                default:
                    // handler will just simply kill, for now
                    this->stop();
                    break;
            }

            // ***** Post Processing *****
            // top ui layer in context for debug
            // TODO: abstract this to ui layer
            // use IDynamo->get_signal() to display Dynamo runtime status
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
            // RenderDynamo calls swap buffers
            m_renderDynamo->flush_frame();
        }
        
    }
    
    void CosmosContext::stop() 
    {
        m_running = false;
    }
}