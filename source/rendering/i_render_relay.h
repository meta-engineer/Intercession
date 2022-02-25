#ifndef RENDER_RELAY_H
#define RENDER_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "rendering/render_packet.h"
#include "rendering/light_packet.h"

namespace pleep
{
    // Are RenderRealys generalizable enough to have an interface?
    // should they only ever need 1 framebuffer/texture from previous steps
    //   do they only ever output to 1 framebuffer/texture?

    // Interface for render pipeline phase
    class IRenderRelay
    {
    public:
        // link framebuffer to output (number of output buffers should be known and setup by dynamo)
        void set_output_fbo_id(unsigned int fboId)
        {
            m_outputFboId = fboId;
        }

        // Accept incoming rendered textures from previous relays?

        // Register dynamo given framebuffer to render out to?

        // submittions are done, invoke draws
        // each relay is different, enfore implementation to avoid confusion
        virtual void render() = 0;


    protected:
        // frambuffer passed from dynamo
        unsigned int m_outputFboId;

        // Subclass should have owned ShaderManagers (possibly more than 1)
    };
}

#endif // RENDER_RELAY_H