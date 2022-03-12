#ifndef FORWARD_RENDER_RELAY_H
#define FORWARD_RENDER_RELAY_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"
#include "rendering/i_render_relay.h"
#include "rendering/shader_manager.h"
#include "rendering/light_source_component.h"

namespace pleep
{
    class ForwardRenderRelay : public IRenderRelay
    {
    public:
        ForwardRenderRelay()
            : m_sm(
                "source/shaders/tangents_ubo.vs",
                "source/shaders/multi_target_hdr.fs"
            )
            , m_normalVisualizerSm(
                "source/shaders/viewspace_normal.vs",
                "source/shaders/vertex_normals.gs",
                "source/shaders/fixed_color.fs"
            )
        {
            // init shader uniforms
            m_sm.activate();
            // guarentee this uniform block will always be block 0?
            glUniformBlockBinding(m_sm.shaderProgram_id, glGetUniformBlockIndex(m_sm.shaderProgram_id, "viewTransforms"), 0);
            m_sm.deactivate();

            m_normalVisualizerSm.activate();
            glUniformBlockBinding(m_normalVisualizerSm.shaderProgram_id, glGetUniformBlockIndex(m_normalVisualizerSm.shaderProgram_id, "viewTransforms"), 0);
            m_normalVisualizerSm.deactivate();

            // create my gl resources
            glGenFramebuffers(1, &m_fboId);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
            
            // LDR component texture buffer
            glGenTextures(1, &m_ldrRenderedTextureId);
            glBindTexture(GL_TEXTURE_2D, m_ldrRenderedTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0); // clear
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ldrRenderedTextureId, 0);

            // HDR component texture buffer
            glGenTextures(1, &m_hdrRenderedTextureId);
            glBindTexture(GL_TEXTURE_2D, m_hdrRenderedTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0); // clear
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_hdrRenderedTextureId, 0);

            // register multiple render targets to this FBO
            unsigned int forwardFboAttachments[2];
            forwardFboAttachments[0] = GL_COLOR_ATTACHMENT0;
            forwardFboAttachments[1] = GL_COLOR_ATTACHMENT1;
            glDrawBuffers(2, forwardFboAttachments);

            // RenderBufferObject for depth (and sometimes stencil)
            glGenRenderbuffers(1, &m_rboId);
            glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_viewWidth, m_viewHeight);
            glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboId);
            
            // check framebuffer
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                PLEEPLOG_ERROR("Forward Pass framebuffer was not complete, rebinding to default");
                glDeleteFramebuffers(1, &m_fboId);
                glDeleteRenderbuffers(1, &m_rboId);
                m_fboId = 0;
            }
            //clear
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~ForwardRenderRelay()
        {
            glDeleteFramebuffers(1, &m_fboId);
            glDeleteRenderbuffers(1, &m_rboId);
            glDeleteTextures(1, &m_ldrRenderedTextureId);
            glDeleteTextures(1, &m_hdrRenderedTextureId);
        }
        
        // I don't need to accept input textures

        // index 0: Rendered Texture of low (normal) dynamic range
        // index 1: Rendered Texture of high (over exposed) dynamic range
        unsigned int get_output_tex_id(unsigned int index = 0) override
        {
            switch(index)
            {
                case 0:
                    return m_ldrRenderedTextureId;
                case 1:
                    return m_hdrRenderedTextureId;
                default:
                    PLEEPLOG_ERROR("Forward render relay only has 2 output textures, could not retrieve index " + std::to_string(index));
                    throw std::range_error("Forward render relay only has 2 output textures, could not retrieve index " + std::to_string(index));
            }
        }
        
        virtual void resize_render_resources() override
        {
            PLEEPLOG_TRACE("Resizing render resources");

            glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

            glBindTexture(GL_TEXTURE_2D, m_ldrRenderedTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glBindTexture(GL_TEXTURE_2D, 0); // clear
            
            glBindTexture(GL_TEXTURE_2D, m_hdrRenderedTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glBindTexture(GL_TEXTURE_2D, 0); // clear

            glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_viewWidth, m_viewHeight);
            glBindRenderbuffer(GL_RENDERBUFFER, 0); // clear

            glBindFramebuffer(GL_FRAMEBUFFER, 0); // clear
        }

        void engage(int* viewportDims) override
        {
            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
            glViewport(
                viewportDims[0],
                viewportDims[1],
                viewportDims[2],
                viewportDims[3]
            );
            // careful, deferred rendering needs x,y,z to be 0
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // options
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // uniforms
            // TODO: ingest camera info
            m_sm.set_vec3("viewPos", m_viewPos);

            // we'll set light uniforms collectively here
            m_numRayLights = 0;
            m_numPointLights = 0;
            m_numSpotLights = 0;
            while (!m_LightSourcePacketQueue.empty())
            {
                LightSourcePacket data = m_LightSourcePacketQueue.front();
                m_LightSourcePacketQueue.pop();
                std::string lightUni;

                switch(data.light.type)
                {
                    case (LightSourceType::ray):
                        if (m_numRayLights >= MAX_RAY_LIGHTS)
                            continue;
                        
                        lightUni = "rLights[" + std::to_string(m_numRayLights) + "]";
                        m_numRayLights++;
                        break;
                    case (LightSourceType::point):
                        if (m_numPointLights >= MAX_POINT_LIGHTS)
                            continue;
                        
                        lightUni = "pLights[" + std::to_string(m_numPointLights) + "]";
                        m_numPointLights++;
                        break;
                    case (LightSourceType::spot):
                        if (m_numSpotLights >= MAX_SPOT_LIGHTS)
                            continue;

                        lightUni = "sLights[" + std::to_string(m_numSpotLights) + "]";
                        m_numSpotLights++;
                        m_sm.set_float(lightUni + ".innerCos", data.light.attributes.x);
                        m_sm.set_float(lightUni + ".outerCos", data.light.attributes.y);
                        break;
                    default:
                        // continues should affect the outer LightSourcePacket loop
                        PLEEPLOG_WARN("Unrecognised light source type, skipping...");
                        continue;
                }
                
                m_sm.set_vec3(lightUni + ".position",
                    data.transform.origin);
                m_sm.set_vec3(lightUni + ".attenuation",
                    data.light.attenuation);
                m_sm.set_vec3(lightUni + ".ambient",
                    data.light.color * data.light.composition.x);
                m_sm.set_vec3(lightUni + ".diffuse",
                    data.light.color * data.light.composition.y);
                m_sm.set_vec3(lightUni + ".specular",
                    data.light.color * data.light.composition.z);
            }
            m_sm.set_int("numRayLights", m_numRayLights);
            m_sm.set_int("numPointLights", m_numPointLights);
            m_sm.set_int("numSpotLights", m_numSpotLights);
            m_sm.deactivate();

            // Render through all models
            while (!m_modelPacketQueue.empty())
            {
                RenderPacket data = m_modelPacketQueue.front();
                m_modelPacketQueue.pop();

                m_sm.activate();
                m_sm.set_mat4("model_to_world", data.transform.get_model_transform());
                data.mesh.invoke_draw(m_sm);
                m_sm.deactivate();
/*
                m_normalVisualizerSm.activate();
                m_normalVisualizerSm.set_mat4("model_to_world", data.transform.get_model_transform());
                data.mesh.invoke_draw(m_normalVisualizerSm);
                m_normalVisualizerSm.deactivate();
*/
            }

            // clear options
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
    private:
        // Foreward pass needs light position/direction, viewPos, shadow transform/farplane/shadowmap
        ShaderManager m_sm;

        // for debugging
        ShaderManager m_normalVisualizerSm;

        // definitions known by my shader
        // IRenderRelay has LightSourcePacketQueue
        const size_t MAX_RAY_LIGHTS   = 1;
        size_t m_numRayLights = 0;
        const size_t MAX_POINT_LIGHTS = 4;
        size_t m_numPointLights = 0;
        const size_t MAX_SPOT_LIGHTS  = 2;
        size_t m_numSpotLights = 0;

        // gl resources
        unsigned int m_fboId;
        unsigned int m_rboId;
        unsigned int m_ldrRenderedTextureId;
        unsigned int m_hdrRenderedTextureId;
    };
}

#endif // FORWARD_RENDER_RELAY_H