#include "cosmos_context.h"

#include "logging/pleep_log.h"

namespace pleep
{
    CosmosContext::CosmosContext() 
        : m_running(false)
    {
        
    }
    
    CosmosContext::CosmosContext(GLFWwindow* appWindow)
        : m_appWindow(appWindow)
    {
        // we have "lease" of api to override callbacks
        //glfwSetFramebufferSizeCallback(m_appWindow, _framebuffer_size_callback);
    }
    
    CosmosContext::~CosmosContext() 
    {
        
    }
    
    void CosmosContext::run() 
    {
        m_running = true;

        // main game loop
        PLEEPLOG_TRACE("Starting main \"game loop\"");
        while (m_running)
        {
            glfwPollEvents();
            // Cosmos should process inputs (or we should call callbacks it registers)

            glClear(GL_COLOR_BUFFER_BIT);

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

                ImGui::Begin("Context Debug");                          // Create a window and append into it.

                ImGui::Text("This window runs above the cosmos");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("checkbox", &checkbox);      // Edit bools storing our window open/close state

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // Render out ui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // call to window api
            if(glfwWindowShouldClose(m_appWindow))
            {
                this->stop();
            }
            
            glfwSwapBuffers(m_appWindow);
        }
        
    }
    
    void CosmosContext::stop() 
    {
        m_running = false;
    }
}