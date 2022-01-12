#include "input_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    InputDynamo::InputDynamo(GLFWwindow* windowApi) 
        : m_windowApi(windowApi)
        , m_closeSignal(false)
    {
    }
    
    InputDynamo::~InputDynamo()
    {
        
    }
    
    void InputDynamo::update(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        glfwPollEvents();

        if (glfwWindowShouldClose(m_windowApi))
        {
            m_closeSignal = true;
            glfwSetWindowShouldClose(m_windowApi, false);
        }
        
        // update each relay

        if (glfwGetKey(m_windowApi, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            m_closeSignal = true;
        }
    }
    
    bool InputDynamo::poll_close_signal() 
    {
        if (m_closeSignal)
        {
            m_closeSignal = false;
            return true;
        }

        return false;
    }
    
}