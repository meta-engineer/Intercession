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
        void submit();
        // process render command queue
        void run_relays(double deltaTime) override;
        // stagger the final flush to allow context to debug
        void flush_frame();

    private:
        // Listening to events::window::RESIZE sent by ControlDynamo
        void _resize_handler(Event resizeEvent);

        void _framebuffer_resize(int width, int height);

        // window api is shared with AppGateway (and other dynamos)
        // hard-code windowApi to glfw for now
        GLFWwindow* m_windowApi;

        // needs a camera component registered as the "main" camera for rendering
        // like a CameraManager, but data only
        // other camera components can be registered for shadow maps, etc...
        //CameraComponent* m_mainCamera
    };
}

#endif // RENDER_DYNAMO_H