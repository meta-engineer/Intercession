#include "render_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    RenderDynamo::RenderDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi)
        : IDynamo(sharedBroker)
        , m_windowApi(windowApi)
    {
        // subscribe to events
        m_sharedBroker->add_listener(METHOD_LISTENER(events::window::RESIZE, RenderDynamo::_resize_handler));
    }
    
    RenderDynamo::~RenderDynamo() 
    {
        // I do not own my api references or event broker
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    void RenderDynamo::submit() 
    {
        // pass mesh/material information to the relay designated by the material
    }
    
    void RenderDynamo::run_relays(double deltaTime) 
    {
        // submited meshes will get assigned to a render relay
        // relays may have a non-greedy process (deferred rendering)
        // here we have finished all submittions and can run through each relay
        UNREFERENCED_PARAMETER(deltaTime);
    }
    
    void RenderDynamo::flush_frame() 
    {
        glfwSwapBuffers(m_windowApi);
    }
    
    void RenderDynamo::_resize_handler(Event resizeEvent) 
    {
        events::window::resize::Params resizeParams = resizeEvent.get_param<events::window::resize::Params>();
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) { width: " + std::to_string(resizeParams.width) + ", height: " + std::to_string(resizeParams.height) + " }");

        _framebuffer_resize(resizeParams.width, resizeParams.height);
    }
    
    void RenderDynamo::_framebuffer_resize(int width, int height) 
    {
        // Do I need access to the GLFWwindow?
        
        unsigned int uWidth  = (unsigned int)width;
        unsigned int uHeight = (unsigned int)height;

        glViewport(0, 0, uWidth, uHeight);

        // assigned cameras/framebuffers also need to get the resize
        //m_mainCamera.set_view_width(uWidth);
        //m_mainCamera.set_view_height(uHeight);
        
    }
}