#ifndef FORWARD_RENDER_RELAY_H
#define FORWARD_RENDER_RELAY_H

//#include "intercession_pch.h"

#include "logging/pleep_log.h"
#include "rendering/i_render_relay.h"
#include "rendering/shader_manager.h"
// This will likely need to be a lightSourcePacket
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
            // TODO: ingest lights from a lightingSynchro
            testLightTransform.origin = glm::vec3(0.0f, 2.0f, -2.0f);

            m_sm.set_int("numPointLights", 1);
            m_sm.set_vec3("pLights[0].position", testLightTransform.origin);
            m_sm.set_vec3("pLights[0].attenuation", testLightSource.attenuation);
            m_sm.set_vec3("pLights[0].ambient",  testLightSource.color * testLightSource.composition.x);
            m_sm.set_vec3("pLights[0].diffuse",  testLightSource.color * testLightSource.composition.y);
            m_sm.set_vec3("pLights[0].specular", testLightSource.color * testLightSource.composition.z);

            m_sm.set_int("numSpotLights", 0);
            m_sm.set_int("numRayLights", 0);

            // guarentee this uniform block will always be block 0?
            glUniformBlockBinding(m_sm.shaderProgram_id, glGetUniformBlockIndex(m_sm.shaderProgram_id, "viewTransforms"), 0);

            m_sm.deactivate();
        }

        void render() override
        {
            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, m_outputFboId);
            glViewport(0,0, 1290,540);
            // careful, deferred rendering needs x,y,z to be 0
            glClearColor(0.5f, 0.6f, 0.8f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // options
            glEnable(GL_DEPTH_TEST);

            // uniforms
            // TODO: ingest camera info
            m_sm.set_vec3("viewPos", glm::vec3(0.0f, 0.0f, 5.0f));
            
            while (!m_packetQueue.empty())
            {
                RenderPacket& data = m_packetQueue.front();

                m_sm.set_mat4("model_to_world", data.transform.get_model_transform());
                data.mesh.invoke_draw(m_sm);

                m_packetQueue.pop();
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_sm.deactivate();
        }

    private:
        // Foreward pass needs light position/direction, viewPos, shadow transform/farplane/shadowmap
        ShaderManager m_sm;

        // TODO: Ingest lights from ECS
        TransformComponent testLightTransform;
        LightSourceComponent testLightSource;
    };
}

#endif // FORWARD_RENDER_RELAY_H