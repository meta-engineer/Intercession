#include "render_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    RenderDynamo::RenderDynamo(GLFWwindow* windowApi)
        : m_windowApi(windowApi)
    {

    }
    
    RenderDynamo::~RenderDynamo() 
    {
        // I do not own my api references
    }
    
    void RenderDynamo::begin_frame() 
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    void RenderDynamo::submit() 
    {
        
    }
    
    void RenderDynamo::end_frame() 
    {
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