#ifndef SCREEN_RENDER_RELAY_H
#define SCREEN_RENDER_RELAY_H

//#include "intercession_pch.h"
#include <memory>

#include "rendering/a_render_relay.h"
#include "rendering/model_library.h"

namespace pleep
{
    class ScreenRenderRelay : public A_RenderRelay
    {
    public:
        ScreenRenderRelay()
            : m_sm(
                "source/shaders/screen_texture.vs",
                "source/shaders/bloom_texture.fs"
            )
            , m_screenSupermesh(ModelLibrary::fetch_screen_supermesh())
        {
            // I don't need uniform buffers
        }

        // index 0: ldr component texture
        // index 1: hdr component texture
        void set_input_tex_id(unsigned int texId, unsigned int index = 0) override
        {
            switch(index)
            {
                case 0:
                    m_ldrScreenTextureId = texId;
                    return;
                case 1:
                    m_hdrScreenTextureId = texId;
                    return;
                default:
                    PLEEPLOG_ERROR("Screen render relay only has 2 input textures, could not access index " + std::to_string(index));
                    throw std::range_error("Screen render relay only has 2 input textures, could not access index " + std::to_string(index));
            }
        }

        // index 0: Final combined display texture
        unsigned int get_output_tex_id(unsigned int index = 0) override
        {
            PLEEPLOG_ERROR("Screen render relay has 0 output textures, draws to framebuffer 0 (screen), could not retrieve index " + std::to_string(index));
            throw std::range_error("Screen render relay has 0 output textures, draws to framebuffer 0 (screen), could not retrieve index " + std::to_string(index));
        }
        
        virtual void resize_render_resources() override
        {
            PLEEPLOG_TRACE("No render resources to resize");
            // I don't have any resources, but I need to implement this to avoid the no-implement warning
        }

        void engage(int* viewportDims) override
        {
            m_sm.activate();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(
                viewportDims[0],
                viewportDims[1],
                viewportDims[2],
                viewportDims[3]
            );
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
            
            m_screenSupermesh->m_submeshes[0]->invoke_draw(m_sm);

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

        // For consistency we'll use a supermesh
        // Should have only 1 submesh: a triangulated square spanning -1 to 1 in x and y
        std::shared_ptr<const Supermesh> m_screenSupermesh;

        // input textures to display
        unsigned int m_ldrScreenTextureId = 0;
        unsigned int m_hdrScreenTextureId = 0;

        // No output render location, outputs to framebuffer 0 (screen)
    };
}

#endif // SCREEN_RENDER_RELAY_H