#ifndef CONTROL_DYNAMO_H
#define CONTROL_DYNAMO_H

//#include "intercession_pch.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "rendering/render_dynamo.h"

namespace pleep
{
    // input has to have feedback (unlike the render dynamo)
    // so its operation should be different, but the concept of a dynamo should be the same
    // an "engine" that is passed entities to "power" using the api
    // "control relays" could be subclassed to power specific components
    // a "controller" component might be a good semantic link (like a material) for input synchro
    // some sort of priority system can be used to shift output targets of the dynamo
    // materials know render relays, controllers know control relays?
    class ControlDynamo
    {
    public:
        ControlDynamo(GLFWwindow* windowApi);
        ~ControlDynamo();

        // poll event queue and process relays
        // return communicates if shouldClose has been called during this update
        bool update(double deltaTime);
        
        // Control Dynamo can directly affect the state of other dynamos
        // this is the most straightforeward solution i could think of
        // hopefully this isn't the start of spaghetti
        void attach_render_dynamo(RenderDynamo* renderDynamo);

    private:
        // take over window user pointer and bind all callbacks below
        void _set_my_window_callbacks();
        void _mouse_move_callback(GLFWwindow* w, double x, double y);
        void _mouse_scroll_callback(GLFWwindow* w, double dx, double dy);
        void _mouse_button_callback(GLFWwindow* w, int button, int action, int mods);
        void _key_callback(GLFWwindow* w, int key, int scancode, int action, int mods);
        void _window_should_close_callback(GLFWwindow* w);
        void _window_size_callback(GLFWwindow* w, int width, int height);

        // receive events from windowing api
        // this should probably be abstracted so that network/ai can be control inputs as well
        GLFWwindow* m_windowApi;

        // Render dynamo I will feed callbacks to
        RenderDynamo* m_attachedRenderDynamo;
    };
}

#endif // CONTROL_DYNAMO_H