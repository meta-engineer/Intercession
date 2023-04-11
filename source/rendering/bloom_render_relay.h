#ifndef BLOOM_RENDER_RELAY_H
#define BLOOM_RENDER_RELAY_H

//#include "intercession_pch.h"
#include <assert.h>
#include "rendering/a_render_relay.h"
#include "logging/pleep_log.h"
#include "rendering/shader_manager.h"
#include "rendering/model_library.h"

namespace pleep
{
    class BloomRenderRelay : public A_RenderRelay
    {
    public:
        BloomRenderRelay()
            : m_sm(
                "source/shaders/screen_texture.vs",
                "source/shaders/gaussian_1d.fs"
            )
            , m_screenSupermesh(ModelLibrary::fetch_supermesh(ModelLibrary::BasicSupermesh::screen))
        {
            // I don't need uniform buffers

            // setup gl objects
            glGenFramebuffers(2, m_bloomFboIds);
            glGenTextures(2, m_bloomRenderedTextureIds);
            for (unsigned int i = 0; i < 2; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFboIds[i]);
                glBindTexture(GL_TEXTURE_2D, m_bloomRenderedTextureIds[i]);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // prevent wrapping
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomRenderedTextureIds[i], 0);

                // check framebuffer
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    PLEEPLOG_ERROR("Bloom hot potato framebuffer " + std::to_string(i) + " was not complete, rebinding to default");
                    glDeleteFramebuffers(1, &m_bloomFboIds[i]);
                    m_bloomFboIds[i] = 0;
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the screen
            }
        }

        ~BloomRenderRelay()
        {
            // toBloom is owned by someone else

            glDeleteFramebuffers(2, m_bloomFboIds);
            glDeleteTextures(2, m_bloomRenderedTextureIds);
        }

        // index 0: texture to apply bloom (blur)
        void set_input_tex_id(unsigned int texId, unsigned int index = 0) override
        {
            switch(index)
            {
                case 0:
                    m_toBloomTextureId = texId;
                    return;
                default:
                    PLEEPLOG_ERROR("Bloom render relay only has 1 input texture, could not access index " + std::to_string(index));
                    throw std::range_error("Bloom render relay only has 1 input texture, could not access index " + std::to_string(index));
            }
        }

        // index 0: Final bloomed (blurred) texture
        unsigned int get_output_tex_id(unsigned int index = 0) override
        {
            switch(index)
            {
                case 0:
                    // blurs are done in pairs of passes, so final should always land on [1]
                    return m_bloomRenderedTextureIds[1];
                default:
                    PLEEPLOG_ERROR("Bloom render relay only has 1 output texture, could not retrieve index " + std::to_string(index));
                    throw std::range_error("Bloom render relay only has 1 output texture, could not retrieve index " + std::to_string(index));
            }
        }
        
        virtual void resize_render_resources() override
        {
            PLEEPLOG_TRACE("Resizing render resources");
            
            for (unsigned int i = 0; i < 2; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFboIds[i]);
                glBindTexture(GL_TEXTURE_2D, m_bloomRenderedTextureIds[i]);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_viewWidth, m_viewHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void engage(int* viewportDims) override
        {
            if (m_numPasses < 1) {
                // TODO: include a passthrough shader for option to have no blur
                PLEEPLOG_ERROR("Cannot perform less than 1 blur passes!");
                assert(m_numPasses >= 1);
            }

            // setup trackers for "hot potato" technique
            unsigned int nextFbo = m_bloomFboIds[0];
            unsigned int nextTex = m_toBloomTextureId;  // for first pass

            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, nextFbo);
            glViewport(
                viewportDims[0],
                viewportDims[1],
                viewportDims[2],
                viewportDims[3]
            );
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // options
            glDisable(GL_DEPTH_TEST);

            // uniforms
            glActiveTexture(GL_TEXTURE0);
            m_sm.set_int("image", 0);

            for (unsigned int i = 0; i < m_numPasses * 2; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, nextFbo);
                m_sm.set_int("horizontal", m_isHorizontalPass);
                m_isHorizontalPass = !m_isHorizontalPass;

                glBindTexture(GL_TEXTURE_2D, nextTex);
                m_screenSupermesh->m_submeshes[0]->invoke_draw(m_sm);

                // setup for next pass
                nextFbo = m_bloomFboIds[(i+1) % 2];
                nextTex = m_bloomRenderedTextureIds[i % 2];
            }
            
            m_sm.deactivate();
        }

    private:
        // blur shader to process hdr texture into bloom texture
        ShaderManager m_sm;
        
        // For consistency we'll use a supermesh
        // Should have only 1 submesh: a triangulated square spanning -1 to 1 in x and y
        std::shared_ptr<const Supermesh> m_screenSupermesh;
        
        // input texture to process (passed in by configuration)
        unsigned int m_toBloomTextureId;

        // used for "hot potato" blurring
        unsigned int m_bloomFboIds[2]{};
        // we'll always output [1]
        unsigned int m_bloomRenderedTextureIds[2]{};

        // track blurring stage
        bool m_isHorizontalPass;

        // each pass means 1 horizontal and 1 vertical render
        const unsigned int m_numPasses = 1;
    };
}

#endif // BLOOM_RENDER_RELAY_H