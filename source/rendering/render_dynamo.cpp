#include "render_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    RenderDynamo::RenderDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi)
        : IDynamo(sharedBroker)
        , m_windowApi(windowApi)
    {

    }
    
    RenderDynamo::~RenderDynamo() 
    {
        // I do not own my api references
    }
    
    void RenderDynamo::prime() 
    {
        IDynamo::prime();

        // fatal error if window api has been destroyed
        if (m_windowApi == nullptr)
        {
            PLEEPLOG_ERROR("Window API reference was unexpectedly NULL");
            throw std::runtime_error("RenderDynamo update found window api reference unexpectedly null");
        }

        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    void RenderDynamo::submit() 
    {
        
    }
    
    void RenderDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void RenderDynamo::flush_frame() 
    {
        glfwSwapBuffers(m_windowApi);
    }
    
    void RenderDynamo::framebuffer_resize_callback(GLFWwindow* window, int width, int height) 
    {
        PLEEPLOG_TRACE("Framebuffer resize triggered");
        // silence warning while not using
        static_cast<void*>(window);
        
        unsigned int uWidth  = (unsigned int)width;
        unsigned int uHeight = (unsigned int)height;

        glViewport(0, 0, uWidth, uHeight);

        // assigned cameras/framebuffers also need to get the resize
        //m_mainCamera.set_view_width(uWidth);
        //m_mainCamera.set_view_height(uHeight);
        
    }
}