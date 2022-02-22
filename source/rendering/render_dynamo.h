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
#include "rendering/mesh.h"
#include "rendering/forward_render_relay.h"
#include "rendering/screen_render_relay.h"

namespace pleep
{
    class RenderDynamo : public IDynamo
    {
    public:
        // any use dynamically attaching apis to a dynamo?
        RenderDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi);
        ~RenderDynamo();

        // methods to process render pipeline
        
        // enter meshes / vertex groups to be rendered
        void submit(RenderPacket data);
        // process render command queue
        void run_relays(double deltaTime) override;
        // stagger the final flush to allow context to debug
        void flush_frame();
        
        // get current viewport origin/sizes
        // viewportDims must be at least 4 int large
        void read_viewport_size(int* viewportDims);

        // update camera/view info
        // dependant on camera origin, rotation, and gimbal_up
        void set_world_to_view(glm::mat4 world_to_view);
        // dependant on camera dimensions, clipping planes, FOV
        void set_projection(glm::mat4 projection);
        // dependant on camera origin
        void set_viewPos(glm::vec3 viewPos);

        // updating camera dimensions should update framebuffer render textures
        void resize_framebuffers(unsigned int width, unsigned int height);

    private:
        // Listening to events::window::RESIZE sent by ControlDynamo
        void _resize_handler(Event resizeEvent);
        // viewport should be proportionally dependant on the window
        void _resize_viewport(int width, int height);
        // however, framebuffer is dependant only on camera
        // (camera may then depend on window, indirectly linking framebuffer to window as well)

        // window api is shared with AppGateway (and other dynamos)
        // hard-code windowApi to glfw for now
        GLFWwindow* m_windowApi;
        // track viewport size to update viewport after shadow passes
        int m_viewportDims[4];

        // Relays encapsulate procedures for a render "pass"
        //   they should output to a framebuffer which can be passed to the next relay
        // It may be overly difficult and complicated to have a fully dynamic relay pipeline
        // more likely RenderDynamo subclasses should just specify specific relays in order
        //   (though the relay types should be publicly available to materials)
        // in the case a material uses a relay unknown to the dynamo, it should log error and use some
        // "default" relay designated by the dynamo
        // Given material structure is standardized/castable/heirarchical we can at least perform SOME render

        // RELAY STEP 1
        std::unique_ptr<ForwardRenderRelay> m_forwardPass;
        unsigned int m_forwardFboId;
        unsigned int m_hdrRenderedTextureId;
        unsigned int m_bloomRenderedTextureId;
        unsigned int m_forwardRboId;
        // this might be unnecessary
        unsigned int m_forwardFboAttachments[2];

        // RELAY STEP 2
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