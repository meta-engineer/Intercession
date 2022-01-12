#include "render_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    RenderDynamo::RenderDynamo(GLFWwindow* windowApi)
        : m_windowApi(windowApi)
    {
        // we have "lease" of api to override callbacks
        //glfwSetFramebufferSizeCallback(m_windowApi, _framebuffer_size_callback);
        
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
}