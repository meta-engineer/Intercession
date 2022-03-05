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
        // stagger the final flush to allow context to debug
        void flush_frame();
        
        // get current viewport origin/sizes
        // viewportDims must be at least 4 int large
        void read_viewport_size(int* viewportDims);

        // updating camera dimensions should update framebuffer render textures
        // also resets framebuffer id's for respective relays
        void resize_framebuffers(unsigned int uWidth, unsigned int uHeight);

    private:
        // Listening to events::window::RESIZE sent by ControlDynamo
        void _resize_handler(Event resizeEvent);
        // viewport should be proportionally dependant on the window
        // however, framebuffer is dependant only on camera
        // (camera may then depend on window, indirectly linking framebuffer to window as well)
        void _resize_viewport(int width, int height);
        // set framebuffers and textures to the pipeline's relays
        void _configure_relay_resources();
        // delete resources during destructor or resize_framebuffers
        void _delete_resources();

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

        // All gl resources are consolidated in the dynamo and shared with relays
        // Relays COULD manage the fbos and dynamo would just call to access the outputs
        //   and pass them to the input of the next relay in the pipeline
        // However, sometimes (like step 1) its not always consistent what/how many
        //   resources are needed to be shared between steps
        // Relays would need meta information about their resources, which the dynamo
        //   would have to parse and try to link with the next relay

        // RELAY STEP 1
        std::unique_ptr<ForwardRenderRelay> m_forwardPass;
        unsigned int m_forwardFboId;
        unsigned int m_ldrRenderedTextureId;
        unsigned int m_hdrRenderedTextureId;
        unsigned int m_forwardRboId;

        // RELAY STEP 2
        std::unique_ptr<BloomRenderRelay> m_bloomPass;
        // we'll always assume that index [0] will be the final, [1] is for internal use
        unsigned int m_bloomFboIds[2]{};
        unsigned int m_bloomRenderedTextureIds[2]{};

        // RELAY STEP 3
        std::unique_ptr<ScreenRenderRelay> m_screenPass;
        // outputs to screen framebuffer (pass framebuffer 0)

        // RenderDynamo maintains a uniform buffer for all relays to use
        unsigned int m_viewTransformUboId;

        // Camera data is not live ( dynamo can't access ECS)
        // but synchro will update each frame (or when neccessary)
        glm::mat4 m_world_to_view;
        glm::mat4 m_projection;
        glm::vec3 m_viewPos;
    };
}

#endif // RENDER_DYNAMO_H