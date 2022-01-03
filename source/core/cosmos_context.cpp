#include "cosmos_context.h"

namespace pleep
{
    CosmosContext::CosmosContext() 
        : m_running(false)
    {
        
    }
    
    CosmosContext::CosmosContext(GLFWwindow* appWindow)
        : m_appWindow(appWindow)
    {
        
    }
    
    CosmosContext::~CosmosContext() 
    {
        
    }
    
    void CosmosContext::run() 
    {
        m_running = true;

        while (m_running)
        {
            // call to window api
            if(glfwWindowShouldClose(m_appWindow))
            {
                this->stop();
            }
            
            glfwSwapBuffers(m_appWindow);
            glfwPollEvents();
        }
        
    }
    
    void CosmosContext::stop() 
    {
        m_running = false;
    }
}