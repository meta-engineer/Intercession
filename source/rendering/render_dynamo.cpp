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
    
    void RenderDynamo::submit(TransformComponent& transform, Mesh& mesh)
    {
        // pass mesh/material information to the relay designated by the meshes material
        UNREFERENCED_PARAMETER(transform);
        UNREFERENCED_PARAMETER(mesh);
    }
    
    void RenderDynamo::run_relays(double deltaTime) 
    {
        // We have finished all submittions and can run through each relay
        // each relay is like a "mini-scene"
        //   initialize the frame
        //   then render through each renderable it has been submitted
        //   then close the frame
        UNREFERENCED_PARAMETER(deltaTime);

        // TODO: update uniform buffer for registered relays


        // run through each relay in my configured order


        // TODO: move this to individual relays
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    void RenderDynamo::flush_frame() 
    {
        glfwSwapBuffers(m_windowApi);
    }
    
    void RenderDynamo::read_viewport_size(int* viewportDims) 
    {
        glGetIntegerv(GL_VIEWPORT, viewportDims);
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