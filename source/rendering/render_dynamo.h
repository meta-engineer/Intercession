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

namespace pleep
{
    class RenderDynamo
    {
    public:
        // any use dynamically attaching apis to a dynamo?
        RenderDynamo(GLFWwindow* windowApi);
        ~RenderDynamo();

        // methods to process render pipeline
        // clear and initialize for frame
        void begin_frame();
        // entry meches / vertex groups to be rendered
        void submit();
        // process render command queue
        void end_frame();
        // stagger the final flush to allow context to debug
        void flush_frame();

        // TODO: how does controlDynamo get here?
        void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
    private:
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