#ifndef CONTROL_DYNAMO_H
#define CONTROL_DYNAMO_H

//#include "intercession_pch.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/i_dynamo.h"
#include "rendering/render_dynamo.h"

namespace pleep
{
    // input has to have feedback (unlike the render dynamo)
    // so its operation should be different, but the concept of a dynamo should be the same
    // an "engine" that is passed entities to "power" using the api
    // each relay will be able to pass data back into the components its fed
    // At the end of a relay (or during) the Dynamo's IDynamo::Signal can be set
    // to communicate non-entity specific feedback
    // Synchro will read this feedback and make judgements.
    // If multiple signals arise, 1 will have to be given priority (hopefully this is a reasonable assumption)

    // "control relays" could be subclassed to power specific components
    // a "controller" component might be a good semantic link (like a material) for input synchro
    // some sort of priority system can be used to shift output targets of the dynamo
    // materials know render relays, controllers know control relays?
    class ControlDynamo : public IDynamo
    {
    public:
        ControlDynamo(GLFWwindow* windowApi);
        ~ControlDynamo();

        // any per-frame init needs to be done
        void prime() override;

        // should control components be "submitted" or statically "registered"
        // "immediate mode" vs "retained mode"
        // NOTE: if you use submit() then it could be added to IDynamo (RenderDynamo also uses it)
        //void submit();

        // poll event queue and process relays
        // THROWS runtime_error if m_windowApi is null
        void run_relays(double deltaTime) override;

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
    };
}

#endif // CONTROL_DYNAMO_H