#ifndef A_RENDER_RELAY_H
#define A_RENDER_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "rendering/camera_packet.h"

namespace pleep
{
    // Are all RenderRelays generalizable enough to have an common base?
    // should they only ever need 1 framebuffer/texture from previous steps
    //   do they only ever output to 1 framebuffer/texture?

    // Abstract base class for render pipeline phase
    class A_RenderRelay
    {
    protected:
        A_RenderRelay() = default;
    public:
        virtual ~A_RenderRelay() = default;

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
        
        // once engage is done they should clear their packets
        // render relays don't have multi-timestep engages, but on engage failure clear is needed
        // if subclasses store additional packets they should override
        virtual void clear()
        {
            // invalidate volatile ecs reference
           m_viewTransform = nullptr;
        }

        // pass in camera to render with (overwrites last submitted camera)
        void submit(CameraPacket data)
        {
            // store reference to camera, because it can change before rendering
            m_viewTransform = &(data.transform);

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

        // subclasses should overload submit(...) with their required packets
        // (make sure to "using A_RenderRelay::submit" to be able to overload)

    protected:
        // Subclass should have owned ShaderManagers (possibly more than 1)

        // Camera info passed from dynamo
        // ECS components are volatile, so copy in needed info
        //   (subclasses can override for more)
        TransformComponent* m_viewTransform = nullptr;
        unsigned int m_viewWidth  = 1024;
        unsigned int m_viewHeight = 1024;

        // subclasses should own containers to submitted packets they require
    };
}

#endif // A_RENDER_RELAY_H