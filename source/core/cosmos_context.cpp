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

        // unique_ptr inits as nullptr
        m_currentCosmos = new Cosmos();

        // construct dynamos
        m_renderDynamo = new RenderDynamo(windowApi);
        m_inputDynamo  = new InputDynamo(windowApi);
        
        // link cosmos synchros with dynamos
        
        // until we can load from file we can manually call methods to build entities in its ecs
        // use imgui input in main loop do add more at runtime
    }
    
    CosmosContext::~CosmosContext() 
    {
        delete m_currentCosmos;

        delete m_renderDynamo;
        delete m_inputDynamo;
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
            // smarter way to get dt?
            thisTimeVal = glfwGetTime();
            deltaTime = thisTimeVal - lastTimeVal;

            // "Event processing is normally done each frame AFTER buffer swapping" ?
            m_inputDynamo->update(deltaTime);

            // TODO: this should be done by synchro? context should have mechanism to signal stop/change
            // InputDynamo should process inputs
            // since dynamos don't know about us, we need to poll states we want
            if(m_inputDynamo->poll_close_signal())
            {
                this->stop();
            }

            // update data for this frame
            m_currentCosmos->update(deltaTime);

            m_renderDynamo->begin_frame();

            // render synchro will submit from ecs
            //m_renderDynamo->submit();

            // once command queue is implemented this will flush them through relays
            m_renderDynamo->end_frame();


            // top ui layer in context for debug
            // TODO: abstract this to ui layer
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
            
            // RenderDynamo calls swap buffers
            m_renderDynamo->flush_frame();
        }
        
    }
    
    void CosmosContext::stop() 
    {
        m_running = false;
    }
}