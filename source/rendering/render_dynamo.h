#ifndef RENDER_DYNAMO_H
#define RENDER_DYNAMO_H

//#include "intercession_pch.h"

// external
#include <memory>
// our "window api"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/i_dynamo.h"
#include "events/event_broker.h"
#include "rendering/render_packet.h"
#include "rendering/light_source_packet.h"
#include "rendering/camera_packet.h"
#include "rendering/mesh.h"
#include "rendering/forward_render_relay.h"
#include "rendering/screen_render_relay.h"
#include "rendering/bloom_render_relay.h"

namespace pleep
{
    class RenderDynamo : public IDynamo
    {
    public:
        // any use dynamically attaching apis to a dynamo?
        RenderDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi);
        ~RenderDynamo();

        // methods to process render pipeline
        
        // pass in meshes / vertex groups to be rendered
        void submit(RenderPacket data);
        // pass in light sources
        void submit(LightSourcePacket data);
        // pass in main camera (overrides last submittion)
        void submit(CameraPacket data);

        // process render command queue
        void run_relays(double deltaTime) override;
        // prepare relays for next frame
        void reset_relays() override;
        // stagger the final flush to allow context to debug
        void flush_frame();
        
        // get current viewport origin/sizes
        // viewportDims must be at least 4 int large
        void read_viewport_size(int* viewportDims);

    private:
        // Listening to events::window::RESIZE sent by ControlDynamo
        void _resize_handler(Event resizeEvent);
        // viewport should be proportionally dependant on the window (respond to resize event)
        // note framebuffers/textures are dependant only on camera (in submit(CameraPacket))
        // (camera may then depend on window, indirectly linking framebuffers to window as well)
        void _resize_viewport(int width, int height);
        // link input/output textures between each relay
        void _configure_relay_resources();

        // window api is shared with AppGateway (and other dynamos)
        // hard-code windowApi to glfw for now
        GLFWwindow* m_windowApi;
        // track viewport size to update viewport after shadow passes
        int m_viewportDims[4];

        // Relays encapsulate procedures for a render "pass"
        //   they should output to a framebuffer which can be passed to the next relay
        // It may be overly difficult and complicated to have a fully dynamic relay pipeline
        // more likely RenderDynamo subclasses should just specify desired relays in order
        //   (though the relay types should be publicly available to materials)
        // in the case a material uses a relay unknown to the dynamo, it should log error and use some
        // "default" relay designated by the dynamo
        // Given the material structure is standardized/castable/heirarchical we can at least perform SOME render

        // Each relay manages its own gl resources
        // dynamo would just call to access the outputs
        //   and pass them to the input of the next relay in the pipeline
        // We still need to explicitly know what/how many outputs/inputs and link them manually

        // RELAY STEP 1
        // forward pass has 0 input textures, 1 fbo, 1 rbo, 2 output textures
        //   it ingests renderables, light sources, and camera changes
        std::unique_ptr<ForwardRenderRelay> m_forwardPass;

        // RELAY STEP 2
        // bloom pass has 1 input texture, 2 fbos, 0 rbos, 1 output texture
        //  it ingests camera changes
        std::unique_ptr<BloomRenderRelay> m_bloomPass;

        // RELAY STEP 3
        // screen pass has 2 input textures, 0 fbos, 0 fbos, 0 output textures
        //   it ingests camera changes
        std::unique_ptr<ScreenRenderRelay> m_screenPass;
        // outputs to screen framebuffer (pass framebuffer 0)

        // RenderDynamo maintains a uniform buffer for all relays to use
        unsigned int m_viewTransformUboId;

        // Data for UBO
        // Camera data is not live (dynamo can't access ECS)
        // but synchro will update each frame, and relay reset should clear at the end of the frame
        std::unique_ptr<CameraPacket> m_cameraData;
    };
}

#endif // RENDER_DYNAMO_H