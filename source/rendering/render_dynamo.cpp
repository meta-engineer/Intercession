#include "render_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    RenderDynamo::RenderDynamo(std::shared_ptr<EventBroker> sharedBroker, GLFWwindow* windowApi)
        : A_Dynamo(sharedBroker)
        , m_windowApi(windowApi)
    {
        PLEEPLOG_TRACE("Start render pipeline setup");

        // subscribe to events
        m_sharedBroker->add_listener(METHOD_LISTENER(events::window::RESIZE, RenderDynamo::_resize_handler));


        // setup UBO
        glGenBuffers(1, &m_viewTransformUboId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewTransformUboId);
        glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
        // set buffer to bind point 0 (relays need to know this)
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_viewTransformUboId, 0, 2 * sizeof(glm::mat4));

        // setup relays
        m_animator    = std::make_unique<AnimationRelay>();
        m_forwardPass = std::make_unique<ForwardRenderRelay>();
        m_bloomPass   = std::make_unique<BloomRenderRelay>();
        m_screenPass  = std::make_unique<ScreenRenderRelay>();
        // configure relays with gl object id's
        _configure_relay_resources();

        // get screen initial size
        read_viewport_size(m_viewportDims);
        // init view dimensions with default screen size until camera sends update

        PLEEPLOG_TRACE("Done render pipeline setup");
    }
    
    RenderDynamo::~RenderDynamo() 
    {
        // I do not own my api references or event broker
        
        // unsubscribe to events
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::window::RESIZE, RenderDynamo::_resize_handler));

        // relays will free framebuffers, textures and renderbuffers
        
        // Uniform buffer shouldn't change size so it doesn't need to be
        //  rebuildable, it is freed only when dynamo ends
        glDeleteBuffers(1, &m_viewTransformUboId);
    }
    
    void RenderDynamo::submit(RenderPacket data)
    {
        // pass transform/mesh/material information to relays
        // we need to dispatch to appropriate relays
        // renderable could have "id" of relay type it wants (what shader it wants)
        //   then if we have that relay in our pipeline we can submit to it
        //   or otherwise use some default
        
        // For now just hardwire to forward relay
        m_forwardPass->submit(data);
    }
    
    void RenderDynamo::submit(DebugRenderPacket data)
    {
        m_forwardPass->submit(data);
    }
    
    void RenderDynamo::submit(LightSourcePacket data) 
    {
        // again, can we implicitly know which relays want this info
        m_forwardPass->submit(data);
    }
    
    void RenderDynamo::submit(CameraPacket data) 
    {
        // store CameraPacket for Uniform Buffer (every submittion overrides)
        // the ECS references are volatile so our pointers shouldn't persist past the frame time
        m_viewTransform = &(data.transform);
        m_viewCamera    = &(data.camera);

        // GL functions don't like height and width of 0
        if (m_viewCamera->viewWidth == 0 || m_viewCamera->viewHeight == 0)
        {
            return;
        }
        // again, i don't know if this is the best solution but,
        // ecs references are volatile so its probably necessary.
        m_forwardPass->submit(data);
        m_screenPass->submit(data);
        m_bloomPass->submit(data);
    }
    
    void RenderDynamo::submit(AnimationPacket data)
    {
        m_animator->submit(data);
    }

    
    void RenderDynamo::run_relays(double deltaTime) 
    {
        // We have finished all submittions and can run through each relay
        // each relay is like a "mini-scene"
        //   initialize the frame
        //   then render through each renderable it has been submitted
        //   then close the frame

        // if there is no camera data then... exit early
        // make sure relays are still reset after!
        if (m_viewTransform == nullptr || m_viewCamera == nullptr
            || m_viewportDims[2] == 0 || m_viewportDims[3] == 0
            || m_viewCamera->viewWidth == 0 || m_viewCamera->viewHeight == 0)
        {
            //PLEEPLOG_WARN("No camera data this frame, skipping without rendering.");
            return;
        }

        // Calculate uniform buffer data
        glm::mat4 world_to_view = get_lookAt(*m_viewTransform, *m_viewCamera);
        glm::mat4 projection    = get_projection(*m_viewCamera);
        // Update uniform buffer for registered relays
        // could optimize further by only resetting if transform/fov changes
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewTransformUboId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,                 sizeof(glm::mat4), &world_to_view);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &projection);

        // run through each relay in my configured order
        GLenum err;
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError before render: " + std::to_string(err)); }
        
        m_animator->engage(deltaTime);
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after animation pass: " + std::to_string(err)); }

        m_forwardPass->engage(m_viewportDims);
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after forward pass: " + std::to_string(err)); }

        m_bloomPass->engage(m_viewportDims);
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after bloom pass: " + std::to_string(err)); }

        m_screenPass->engage(m_viewportDims);
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after screen pass: " + std::to_string(err)); }
    }
    
    void RenderDynamo::reset_relays()
    {
        // TODO: render relays are not setup to have fixed timestep (multiple iterations per frame)
        // so they clear themselves automatically
        m_animator->clear();
        m_forwardPass->clear();
        m_bloomPass->clear();
        m_screenPass->clear();

        // invalidate volatile camera data from ecs
        m_viewTransform = nullptr;
        m_viewCamera = nullptr;

        this->flush_frame();
    }
    
    void RenderDynamo::flush_frame() 
    {
        glfwSwapBuffers(m_windowApi);
    }
    
    void RenderDynamo::read_viewport_size(int* viewportDims) 
    {
        glGetIntegerv(GL_VIEWPORT, viewportDims);
    }
    
    void RenderDynamo::_resize_handler(EventMessage resizeEvent) 
    {
        events::window::RESIZE_params resizeParams;
        resizeEvent >> resizeParams;
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) { width: " + std::to_string(resizeParams.width) + ", height: " + std::to_string(resizeParams.height) + " }");

        _resize_viewport(resizeParams.width, resizeParams.height);
    }
    
    void RenderDynamo::_resize_viewport(int width, int height) 
    {
        unsigned int uWidth  = (unsigned int)width;
        unsigned int uHeight = (unsigned int)height;

        // relays will use this info to set viewport appropriately
        m_viewportDims[0] = 0;
        m_viewportDims[1] = 0;
        m_viewportDims[2] = uWidth;
        m_viewportDims[3] = uHeight;
    }
    
    void RenderDynamo::_configure_relay_resources() 
    {
        // forward pass needs nothing

        // bloom pass needs bright component of forward pass output
        m_bloomPass->set_input_tex_id(m_forwardPass->get_output_tex_id(1));

        // screen pass needs normal component of forward pass and bloom component from bloom pass
        m_screenPass->set_input_tex_id(m_forwardPass->get_output_tex_id(0), 0);
        m_screenPass->set_input_tex_id(m_bloomPass->get_output_tex_id(0), 1);
        
        PLEEPLOG_TRACE("Framebuffers reconfigured successfully");
    }
}