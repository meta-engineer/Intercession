#ifndef SCREEN_RENDER_RELAY_H
#define SCREEN_RENDER_RELAY_H

//#include "intercession_pch.h"
#include <memory>

#include "rendering/i_render_relay.h"
#include "rendering/model_builder.h"
#include "rendering/vertex_group.h"

namespace pleep
{
    class ScreenRenderRelay : public IRenderRelay
    {
    public:
        ScreenRenderRelay()
            : m_sm(
                "source/shaders/screen_texture.vs",
                "source/shaders/bloom_texture.fs"
            )
            , m_screenPlane(model_builder::create_screen_plane())
        {
            // I don't need uniform buffers
        }

        void render() override
        {
            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, m_outputFboId);
            glViewport(0,0, m_viewWidth,m_viewHeight);
            glClearColor(0.6f, 0.2f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // options
            glDisable(GL_DEPTH_TEST);

            // uniforms
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_ldrScreenTextureId);
            m_sm.set_int("screenTexture", 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_hdrScreenTextureId);
            m_sm.set_int("bloomTexture", 1);
            
            m_screenPlane->invoke_draw();

            m_sm.deactivate();
        }

        // provide texture ids to use for output, if hdr component is 0 it will have no effect
        void set_input_texture(unsigned int ldrId, unsigned int hdrId = 0)
        {
            m_ldrScreenTextureId = ldrId;
            m_hdrScreenTextureId = hdrId;
        }

    private:
        // screen pass needs "screenTexture" texture id from previous a relay
        ShaderManager m_sm;

        // again, can't let objects with gl memory be copied...
        std::shared_ptr<VertexGroup> m_screenPlane;

        unsigned int m_ldrScreenTextureId = 0;
        unsigned int m_hdrScreenTextureId = 0;
    };
}

#endif // SCREEN_RENDER_RELAY_H