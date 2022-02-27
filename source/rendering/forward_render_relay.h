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
        {
            // init shader uniforms
            m_sm.activate();

            // guarentee this uniform block will always be block 0?
            glUniformBlockBinding(m_sm.shaderProgram_id, glGetUniformBlockIndex(m_sm.shaderProgram_id, "viewTransforms"), 0);

            m_sm.deactivate();
        }

        void render() override
        {
            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, m_outputFboId);
            glViewport(0,0, m_viewWidth,m_viewHeight);
            // careful, deferred rendering needs x,y,z to be 0
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // options
            glEnable(GL_DEPTH_TEST);

            // uniforms
            // TODO: ingest camera info
            m_sm.set_vec3("viewPos", m_viewPos);

            // we'll set light uniforms collectively here
            m_numRayLights = 0;
            m_numPointLights = 0;
            m_numSpotLights = 0;
            while (!m_LightSourcePacketQueue.empty())
            {
                LightSourcePacket& data = m_LightSourcePacketQueue.front();
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

            // Render through all models
            while (!m_modelPacketQueue.empty())
            {
                RenderPacket& data = m_modelPacketQueue.front();
                m_modelPacketQueue.pop();

                m_sm.set_mat4("model_to_world", data.transform.get_model_transform());
                data.mesh.invoke_draw(m_sm);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_sm.deactivate();
        }
        
    private:
        // Foreward pass needs light position/direction, viewPos, shadow transform/farplane/shadowmap
        ShaderManager m_sm;

        // definitions known by my shader
        // IRenderRelay has LightSourcePacketQueue
        const size_t MAX_RAY_LIGHTS   = 1;
        size_t m_numRayLights = 0;
        const size_t MAX_POINT_LIGHTS = 4;
        size_t m_numPointLights = 0;
        const size_t MAX_SPOT_LIGHTS  = 2;
        size_t m_numSpotLights = 0;
    };
}

#endif // FORWARD_RENDER_RELAY_H