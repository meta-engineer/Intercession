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

    private:
        // window api is shared with AppGateway (and other dynamos)
        // hard-code windowApi to glfw for now
        GLFWwindow* m_windowApi;
    };
}

#endif // RENDER_DYNAMO_H