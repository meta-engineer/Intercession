#ifndef RENDER_RELAY_H
#define RENDER_RELAY_H

//#include "intercession_pch.h"
#include <queue>

#include "rendering/render_packet.h"
#include "rendering/light_source_packet.h"
#include "rendering/camera_packet.h"

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

        // Subclasses should accept rendered textures from previous relays

        // submittions are done, invoke draws
        // each relay is different, enfore implementation to avoid confusion
        virtual void render() = 0;

        // pass in camera to render with (overwrites last cubmitted camera)
        void submit(CameraPacket data)
        {
            m_viewPos = data.transform.origin;
            m_viewWidth = data.camera.viewWidth;
            m_viewHeight = data.camera.viewHeight;
        }
        
        // Accept Mesh data to render to
        // does EVERY kind of RenderRelay need this?
        void submit(RenderPacket data)
        {
            m_modelPacketQueue.push(data);
        }

        void submit(LightSourcePacket data)
        {
            m_LightSourcePacketQueue.push(data);
        }

    protected:
        // framebuffer passed from dynamo
        unsigned int m_outputFboId;

        // Camera info passed from dynamo
        // ECS components are volatile, so copy in needed info
        //   (subclasses can override for more)
        glm::vec3 m_viewPos;
        unsigned int m_viewWidth;
        unsigned int m_viewHeight;

        // collect packets during render submitting
        std::queue<RenderPacket> m_modelPacketQueue;
        std::queue<LightSourcePacket> m_LightSourcePacketQueue;

        // Subclass should have owned ShaderManagers (possibly more than 1)
    };
}

#endif // RENDER_RELAY_H