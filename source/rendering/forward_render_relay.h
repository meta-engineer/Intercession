#ifndef FORWARD_RENDER_RELAY_H
#define FORWARD_RENDER_RELAY_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"
#include "rendering/a_render_relay.h"
#include "rendering/shader_manager.h"
#include "rendering/render_packet.h"
#include "rendering/light_source_packet.h"
#include "rendering/model_cache.h"

#define RENDER_COLLIDERS
//#define RENDER_MESHES

namespace pleep
{
    class ForwardRenderRelay : public A_RenderRelay
    {
    public:
        ForwardRenderRelay()
            : m_sm(
                "source/shaders/tangents_ubo.vs",
                "source/shaders/new_material_hdr.fs"
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
            glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // options
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // culling (with correct index orientation)
            //glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            // wireframe
            //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
            // enable stencil operations
            //glEnable(GL_STENCIL_TEST);

            // uniforms
            // TODO: ingest camera info
            m_sm.set_vec3("viewPos", m_viewPos);

            // we'll set light uniforms collectively here
            m_numRayLights = 0;
            m_numPointLights = 0;
            m_numSpotLights = 0;
            for (std::vector<LightSourcePacket>::iterator packet_it = m_lightSourcePackets.begin(); packet_it != m_lightSourcePackets.end(); packet_it++)
            {
                LightSourcePacket& data = *packet_it;
                std::string lightUni;

                switch(data.light.type)
                {
                    case (LightSourceType::ray):
                        if (m_numRayLights >= MAX_RAY_LIGHTS)
                            continue;
                        
                        lightUni = "rLights[" + std::to_string(m_numRayLights) + "]";
                        m_sm.set_vec3(lightUni + ".direction", data.transform.get_heading());
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
                        m_sm.set_vec3(lightUni + ".direction", data.transform.get_heading());
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
            m_sm.set_int("numRayLights", static_cast<int>(m_numRayLights));
            m_sm.set_int("numPointLights", static_cast<int>(m_numPointLights));
            m_sm.set_int("numSpotLights", static_cast<int>(m_numSpotLights));
            m_sm.deactivate();
#ifdef RENDER_MESHES
            // Render through all meshes
            for (std::vector<RenderPacket>::iterator packet_it = m_modelPackets.begin(); packet_it != m_modelPackets.end(); packet_it++)
            {
                RenderPacket& data = *packet_it;

                // check nullptr
                if (data.renderable.meshData == nullptr) continue;

                m_sm.activate();
                m_sm.set_mat4("model_to_world", data.transform.get_model_transform() * data.renderable.localTransform.get_model_transform());
                for (unsigned int i = 0; i < data.renderable.meshData->m_submeshes.size(); i++)
                {
                    // if there are no materials... do nothing? get debug material from ModelCache?
                    // TEMP: "highlight" will make it blackout (no materials)
                    if (data.renderable.materials.empty() || data.renderable.highlight)
                    {
                        _set_material_textures(m_sm, nullptr);
                    }
                    else
                    {
                        // if there aren't enough materials for all submeshes,
                        // the un-paired submeshes will use the last-most material
                        _set_material_textures(m_sm, data.renderable.materials[i < data.renderable.materials.size() ? i : (data.renderable.materials.size() - 1)]);
                    }
                    
                    data.renderable.meshData->m_submeshes[i]->invoke_draw(m_sm);
                }
                m_sm.deactivate();
            }
#endif // RENDER_MESHES
#ifdef RENDER_COLLIDERS
            // render debug packets
            for (std::vector<DebugRenderPacket>::iterator packet_it = m_debugPackets.begin(); packet_it != m_debugPackets.end(); packet_it++)
            {
                DebugRenderPacket& data = *packet_it;

                m_sm.activate();
                m_sm.set_mat4("model_to_world", data.transform);
                
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);

                std::shared_ptr<const Supermesh> supermesh = ModelCache::fetch_supermesh(data.supermeshType);
                for (auto submesh : supermesh->m_submeshes)
                {
                    // material?
                    _set_material_textures(m_sm, nullptr);

                    submesh->invoke_draw(m_sm);
                }
                
                glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
                m_sm.deactivate();
            }
#endif // RENDER_COLLIDERS
            // clear options
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // explicitly inherit CameraPakcet submit
        using A_RenderRelay::submit;
        
        // Accept Mesh data to render to
        // Should there be a limit to this?
        void submit(RenderPacket data)
        {
            m_modelPackets.push_back(data);
        }

        void submit(DebugRenderPacket data)
        {
            m_debugPackets.push_back(data);
        }

        void submit(LightSourcePacket data)
        {
            m_lightSourcePackets.push_back(data);
        }

        void clear() override
        {
            m_modelPackets.clear();
            m_debugPackets.clear();
            m_lightSourcePackets.clear();
        }
        
    private:
        // Foreward pass needs light position/direction, viewPos, shadow transform/farplane/shadowmap
        ShaderManager m_sm;

        // for debugging
        ShaderManager m_normalVisualizerSm;

        // definitions known by my shader
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
        
        // collect packets during render submitting
        std::vector<RenderPacket> m_modelPackets;
        std::vector<DebugRenderPacket> m_debugPackets;
        std::vector<LightSourcePacket> m_lightSourcePackets;

        void _set_material_textures(ShaderManager& sm, std::shared_ptr<const Material> material)
        {
            // new materials only have 1 of each texture type!
            // TODO: store and check last material name that was set to avoid overwriting the same data?
            unsigned int texIndex = 0; // TODO: this could overflow GL_TEXTURE31
            // track textures set by material to know which defaults to set after
            std::unordered_map<TextureType, bool> setTextures;
            // would it be slower to set all defaults BEFORE material?
            //   or is tracking on cpu to avoid more opengl calls optimal?

            // re-use one string to avoid as many string destructions as possible
            std::string uniformName = "";
            
            if (material) // incase of nullptr
            {
                for (auto materialIt = material->m_textures.begin(); materialIt != material->m_textures.end(); materialIt++)
                {
                    glActiveTexture(GL_TEXTURE0 + texIndex); // gl addresses can do pointer arithmatic
                    glBindTexture(GL_TEXTURE_2D, materialIt->second.get_id());

                    // shader material format is: material.TYPESTR
                    // Careful! Not all material types might be accepted by the shader
                    uniformName = "material."; uniformName.append(TEXTURETYPE_TO_STR(materialIt->first));
                    sm.set_int(uniformName, texIndex);
                    setTextures[materialIt->first] = true;
                    
                    // set flags for optional maps
                    switch(materialIt->first)
                    {
                        case(TextureType::normal):
                        case(TextureType::height):
                            uniformName = "material.use_"; uniformName.append(TEXTURETYPE_TO_STR(materialIt->first));
                            sm.set_bool(uniformName, true);
                            break;
                        default:
                            // no flag needed
                            break;
                    }

                    texIndex++;
                }
                // texIndex should be incremented to unused id after loop
            }
            
            // If a material has less than expected textures (i.e. no specular) it will use the last bound one.
            // Instead we should use 0 (all black). We could implement a global null texture for debug visibility
            glActiveTexture(GL_TEXTURE0 + texIndex);
            glBindTexture(GL_TEXTURE_2D, 0); // texture_id 0 should be black
            // check for textures this shader requires
            if (setTextures[TextureType::diffuse] == false)
            {
                uniformName = "material."; uniformName.append(TEXTURETYPE_TO_STR(TextureType::diffuse));
                sm.set_int(uniformName, texIndex);
            }
            if (setTextures[TextureType::specular] == false)
            {
                uniformName = "material."; uniformName.append(TEXTURETYPE_TO_STR(TextureType::specular));
                sm.set_int(uniformName, texIndex);
            }
            if (setTextures[TextureType::normal] == false)
            {
                uniformName = "material.use_"; uniformName.append(TEXTURETYPE_TO_STR(TextureType::normal));
                sm.set_bool(uniformName, false);
            }
            if (setTextures[TextureType::height] == false)
            {
                uniformName = "material.use_"; uniformName.append(TEXTURETYPE_TO_STR(TextureType::height));
                sm.set_bool(uniformName, false);
            }
            if (setTextures[TextureType::emissive] == false)
            {
                uniformName = "material."; uniformName.append(TEXTURETYPE_TO_STR(TextureType::emissive));
                sm.set_int(uniformName, texIndex);
            }


            // environment map (data.renderable.environmentCubemap.id)
            // DISABLED for now
            // use explicit environment map if id is not default
            //if (environmentCubemap.id != 0 && environmentCubemap.type == TextureType::cube_map)
            //{
            //    i++;
            //    glActiveTexture(GL_TEXTURE0 + i);
            //    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubemap.id);
            //    // cube_maps (plural)
            //    sm.set_int("environmentCubemap", i);
            //    sm.set_bool("environmentCubemap_enable", true);
            //}
            //else
            //{

            texIndex++;
            glActiveTexture(GL_TEXTURE0 + texIndex);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // texture_id 0 should be black
            // must bind black texture (0) so NVIDIA is happy
            sm.set_int("environmentCubemap", texIndex);
            sm.set_int("environmentCubemap_enable", false);

            //}
        }
    };
}

#endif // FORWARD_RENDER_RELAY_H