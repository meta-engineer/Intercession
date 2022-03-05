#ifndef BLOOM_RENDER_RELAY_H
#define BLOOM_RENDER_RELAY_H

//#include "intercession_pch.h"
#include "rendering/i_render_relay.h"
#include "logging/pleep_log.h"
#include "rendering/shader_manager.h"
#include "rendering/model_builder.h"
#include "rendering/vertex_group.h"

namespace pleep
{
    class BloomRenderRelay : public IRenderRelay
    {
    public:
        BloomRenderRelay()
            : m_sm(
                "source/shaders/screen_texture.vs",
                "source/shaders/gaussian_1d.fs"
            )
            , m_screenPlane(model_builder::create_screen_plane())
        {
            // I don't need uniform buffers
        }


        void render() override
        {
            if (m_numPasses < 2) {
                PLEEPLOG_WARN("Cannot perform less than 2 blur passes, performing 2 anyway");
            }

            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, m_outputFboId);
            glViewport(0,0, m_viewWidth,m_viewHeight);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // options
            glDisable(GL_DEPTH_TEST);

            // uniforms
            glActiveTexture(GL_TEXTURE0);
            m_sm.set_int("image", 0);

            // setup for first pass
            unsigned int nextTex = m_toBloomTextureId;
            unsigned int nextFbo = m_bloomProcessingFboId1;

            for (unsigned int i = 0; i < m_numPasses-1; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, nextFbo);
                m_sm.set_int("horizontal", m_isHorizontalPass);
                m_isHorizontalPass = !m_isHorizontalPass;

                glBindTexture(GL_TEXTURE_2D, nextTex);
                m_screenPlane->invoke_draw();

                // setup for even passes
                if (i%2 == 0)
                {
                    nextTex = m_bloomProcessingTexId1;
                    nextFbo = m_bloomProcessingFboId2;
                }
                // setup for odd passes (except first)
                else
                {
                    nextTex = m_bloomProcessingTexId2;
                    nextFbo = m_bloomProcessingFboId1;
                }
            }

            // last pass to output fbo
            glBindFramebuffer(GL_FRAMEBUFFER, m_outputFboId);
            m_sm.set_int("horizontal", m_isHorizontalPass);
            m_isHorizontalPass = !m_isHorizontalPass;
            
            glBindTexture(GL_TEXTURE_2D, nextTex);
            m_screenPlane->invoke_draw();
            
            m_sm.deactivate();
        }

        // provide texture to be bloomed (blurred)
        void set_input_texture(unsigned int toBloomId)
        {
            m_toBloomTextureId = toBloomId;
        }

        // in addition to output_fbo_id, bloom also needs internal "hot potato" framebuffers
        // Relay could manage it itself, but would need to be looped into dynamo's framebuffer updates
        void set_internal_fbo_ids(unsigned int processingFbo1, unsigned int processingFbo2)
        {
            m_bloomProcessingFboId1 = processingFbo1;
            m_bloomProcessingFboId2 = processingFbo2;
        }
        
        void set_internal_render_texture_ids(unsigned int processingTex1, unsigned int processingTex2)
        {
            m_bloomProcessingTexId1 = processingTex1;
            m_bloomProcessingTexId2 = processingTex2;
        }

    private:
        // blur shader to process hdr texture into bloom texture
        ShaderManager m_sm;
        
        std::shared_ptr<VertexGroup> m_screenPlane;
        
        // input texture to process
        unsigned int m_toBloomTextureId;

        // remember we get m_outputFboId from IRenderRelay

        // used for "hot potato" blurring
        unsigned int m_bloomProcessingFboId1;
        unsigned int m_bloomProcessingFboId2;
        unsigned int m_bloomProcessingTexId1;
        unsigned int m_bloomProcessingTexId2;

        // track blurring stage
        bool m_isHorizontalPass;

        const unsigned int m_numPasses = 2;
    };
}

#endif // BLOOM_RENDER_RELAY_H