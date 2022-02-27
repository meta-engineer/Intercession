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

        PLEEPLOG_TRACE("Setup render pipeline");
        // setup UBO
        glGenBuffers(1, &m_viewTransformUboId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewTransformUboId);
        glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
        // set buffer to bind point 0 (relays need to know this)
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_viewTransformUboId, 0, 2 * sizeof(glm::mat4));

        // setup relays
        m_forwardPass = std::make_unique<ForwardRenderRelay>();
        m_screenPass = std::make_unique<ScreenRenderRelay>();

        // get screen initial size
        read_viewport_size(m_viewportDims);
        // init view dimensions with default screen size until camera sends update

        // gl object id's should init as 0
        // setup framebuffers then
        // configure relays with new object id's

        resize_framebuffers(m_viewportDims[2], m_viewportDims[3]);
        PLEEPLOG_TRACE("Render pipeline config done.");
    }
    
    RenderDynamo::~RenderDynamo() 
    {
        // I do not own my api references or event broker

        glDeleteFramebuffers(1, &m_forwardFboId);
        glDeleteTextures(1, &m_hdrRenderedTextureId);
        glDeleteTextures(1, &m_bloomRenderedTextureId);
        glDeleteRenderbuffers(1, &m_forwardRboId);

        glDeleteBuffers(1, &m_viewTransformUboId);
    }
    
    void RenderDynamo::submit(RenderPacket data)
    {
        // pass transform/mesh/material information to relays
        // we need to dispatch to appropriat relays
        // mesh material could have "id" of relay type it wants (what shader it wants)
        //   then if we have that relay in our pipeline we can submit to it
        // Otherwise use some default
        
        // we don't have materials, so for now just hardwire to relays
        m_forwardPass->submit(data);
    }
    
    void RenderDynamo::submit(LightSourcePacket data) 
    {
        // again, can we implicitly know which relays want this info
        m_forwardPass->submit(data);
    }
    
    void RenderDynamo::submit(CameraPacket data) 
    {
        // again, i don't know if this is the best solution but,
        // ecs references are volatile so its probably necessary.
        m_forwardPass->submit(data);
        m_screenPass->submit(data);

        // I also need to save some data to set UBO
        // these calulated mat4s are no longer volatile, so they're ok
        m_world_to_view = get_lookAt(data.transform, data.camera);
        m_projection    = get_projection(data.camera);
    }

    
    void RenderDynamo::run_relays(double deltaTime) 
    {
        // We have finished all submittions and can run through each relay
        // each relay is like a "mini-scene"
        //   initialize the frame
        //   then render through each renderable it has been submitted
        //   then close the frame
        UNREFERENCED_PARAMETER(deltaTime);

        // Update uniform buffer for registered relays
        // could optimize further by only resetting if transform/fov changes
        glBindBuffer(GL_UNIFORM_BUFFER, m_viewTransformUboId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,                 sizeof(glm::mat4), &m_world_to_view);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &m_projection);

        // run through each relay in my configured order
        GLenum err;
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError before render: " + std::to_string(err)); }

        m_forwardPass->render();
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after forward pass: " + std::to_string(err)); }

        m_screenPass->render();
        err = glGetError();
        if (err) { PLEEPLOG_ERROR("glError after screen pass: " + std::to_string(err)); }
    }
    
    void RenderDynamo::flush_frame() 
    {
        glfwSwapBuffers(m_windowApi);
    }
    
    void RenderDynamo::read_viewport_size(int* viewportDims) 
    {
        glGetIntegerv(GL_VIEWPORT, viewportDims);
    }
    
    void RenderDynamo::resize_framebuffers(unsigned int uWidth, unsigned int uHeight) 
    {
        // Naive solution, just delete the textures and remake them
        glDeleteFramebuffers(1, &m_forwardFboId);
        glDeleteTextures(1, &m_hdrRenderedTextureId);
        glDeleteTextures(1, &m_bloomRenderedTextureId);
        glDeleteRenderbuffers(1, &m_forwardRboId);


        // setup Framebuffers
        glGenFramebuffers(1, &m_forwardFboId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_forwardFboId);

        // HDR texture buffer
        glGenTextures(1, &m_hdrRenderedTextureId);
        glBindTexture(GL_TEXTURE_2D, m_hdrRenderedTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, uWidth, uHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0); // clear
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrRenderedTextureId, 0);

        // BLOOM texture buffer
        glGenTextures(1, &m_bloomRenderedTextureId);
        glBindTexture(GL_TEXTURE_2D, m_bloomRenderedTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, uWidth, uHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0); // clear
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_bloomRenderedTextureId, 0);

        // register multiple render targets to this FBO
        m_forwardFboAttachments[0] = GL_COLOR_ATTACHMENT0;
        m_forwardFboAttachments[1] = GL_COLOR_ATTACHMENT1;
        glDrawBuffers(2, m_forwardFboAttachments);

        // RenderBufferObject for depth (and sometimes stencil)
        glGenRenderbuffers(1, &m_forwardRboId);
        glBindRenderbuffer(GL_RENDERBUFFER, m_forwardRboId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, uWidth, uHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_forwardRboId);
        
        // check framebuffer
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            PLEEPLOG_ERROR("Screen Framebuffer was not complete, rebinding to default");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen

        m_forwardPass->set_output_fbo_id(m_forwardFboId);
        m_screenPass->set_output_fbo_id(0);
        m_screenPass->set_input_screen_textures(m_hdrRenderedTextureId, m_bloomRenderedTextureId);
        
        PLEEPLOG_TRACE("Framebuffers constructed successfully");
    }
    
    void RenderDynamo::_resize_handler(Event resizeEvent) 
    {
        events::window::resize::Params resizeParams = resizeEvent.get_param<events::window::resize::Params>();
        
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) { width: " + std::to_string(resizeParams.width) + ", height: " + std::to_string(resizeParams.height) + " }");

        _resize_viewport(resizeParams.width, resizeParams.height);
    }
    
    void RenderDynamo::_resize_viewport(int width, int height) 
    {
        unsigned int uWidth  = (unsigned int)width;
        unsigned int uHeight = (unsigned int)height;

        // always have viewport fill window 100%
        glViewport(0, 0, uWidth, uHeight);

        // remember original viewport to reset after relays modify it
        m_viewportDims[0] = 0;
        m_viewportDims[1] = 0;
        uWidth = uWidth;
        uHeight = uHeight;
    }
}