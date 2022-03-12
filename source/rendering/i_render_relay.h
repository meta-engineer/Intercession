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
        // Set rendered texture resources for input to this relay
        virtual void set_input_tex_id(unsigned int texId, unsigned int index = 0)
        {
            UNREFERENCED_PARAMETER(texId);
            UNREFERENCED_PARAMETER(index);
            PLEEPLOG_WARN("No implementation for set_input_tex_id(). Default behaviour is ignoring the set texture.");
            return;
        }

        // Get id of rendered textures this relay outputs
        // It may be useful to get "fingerprint" of how many textures this relay has, and their content
        virtual unsigned int get_output_tex_id(unsigned int index = 0)
        {
            UNREFERENCED_PARAMETER(index);
            PLEEPLOG_WARN("No implementation for get_output_tex_id(). Default behaviour is returning 0.");
            return 0;
        }
        
        // set framebuffer/texture resolutions with m_viewWidth & m_viewHeight
        virtual void resize_render_resources()
        {
            PLEEPLOG_WARN("No implementation for resize_render_resources(). Default behaviour is nothing");
        }

        // submittions are done, invoke draws
        // viewportDims[4] should be used to set default viewport
        // each relay is different, enforce implementation to avoid confusion
        virtual void engage(int* viewportDims) = 0;

        // pass in camera to render with (overwrites last cubmitted camera)
        void submit(CameraPacket data)
        {
            m_viewPos = data.transform.origin;

            // if camera dimensions change (either new camera or modified current camera)
            // render resources need to be resized
            if (m_viewWidth != data.camera.viewWidth || m_viewHeight != data.camera.viewHeight)
            {
                PLEEPLOG_TRACE("Detected new camera size {" 
                    + std::to_string(data.camera.viewWidth) + ", " + std::to_string(data.camera.viewHeight)
                    + "} (from previous {" + std::to_string(m_viewWidth) + ", " + std::to_string(m_viewHeight) + "})"
                );
                m_viewWidth = data.camera.viewWidth;
                m_viewHeight = data.camera.viewHeight;

                // dispatch to subclasses override
                this->resize_render_resources();
            }
        }
        
        // Accept Mesh data to render to
        // Should there be a limit to this?
        void submit(RenderPacket data)
        {
            m_modelPacketQueue.push(data);
        }

        void submit(LightSourcePacket data)
        {
            m_LightSourcePacketQueue.push(data);
        }

    protected:
        // Subclass should have owned ShaderManagers (possibly more than 1)

        // Camera info passed from dynamo
        // ECS components are volatile, so copy in needed info
        //   (subclasses can override for more)
        glm::vec3 m_viewPos       = glm::vec3(0.0f);
        unsigned int m_viewWidth  = 1024;
        unsigned int m_viewHeight = 1024;

        // collect packets during render submitting
        std::queue<RenderPacket> m_modelPacketQueue;
        std::queue<LightSourcePacket> m_LightSourcePacketQueue;
    };
}

#endif // RENDER_RELAY_H