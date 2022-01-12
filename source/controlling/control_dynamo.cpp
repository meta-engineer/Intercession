#include "control_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    ControlDynamo::ControlDynamo(GLFWwindow* windowApi) 
        : m_windowApi(windowApi)
        , m_closeSignal(false)
    {
    }
    
    ControlDynamo::~ControlDynamo()
    {
        
    }
    
    void ControlDynamo::update(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        glfwPollEvents();

        if (glfwWindowShouldClose(m_windowApi))
        {
            // maybe this should trigger some ui instead, and call appropriate shutdown/logoff
            m_closeSignal = true;
            glfwSetWindowShouldClose(m_windowApi, false);
        }
        
        // update each relay

        // for development
        if (glfwGetKey(m_windowApi, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            m_closeSignal = true;
        }
    }
    
    bool ControlDynamo::poll_close_signal() 
    {
        if (m_closeSignal)
        {
            m_closeSignal = false;
            return true;
        }

        return false;
    }
    
}