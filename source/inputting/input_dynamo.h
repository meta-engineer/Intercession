#ifndef INPUT_DYNAMO_H
#define INPUT_DYNAMO_H

//#include "intercession_pch.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/a_dynamo.h"
#include "inputting/spacial_input_packet.h"
#include "inputting/raw_input_buffer.h"
#include "inputting/spacial_input_relay.h"

namespace pleep
{
    // The input dynamo converts the raw device input from the windowing api
    // into specific "schemes" within each relay.
    // Each frame entities with input components will be updated by their
    // respective relay, recieving the translation of the polled raw input
    // Other Dynamos (BehaviorsDynamo) can then use the input component as they wish
    class InputDynamo : public A_Dynamo
    {
    public:
        // takes over window user pointer and binds callbacks
        InputDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi);
        ~InputDynamo();

        void submit(SpacialInputPacket data);

        // poll window event queue and process relays
        // THROWS runtime_error if m_windowApi is null
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        void _set_my_window_callbacks();
        void _mouse_move_callback(GLFWwindow* w, double x, double y);
        void _mouse_scroll_callback(GLFWwindow* w, double dx, double dy);
        void _mouse_button_callback(GLFWwindow* w, int button, int action, int mods);
        void _key_callback(GLFWwindow* w, int key, int scancode, int action, int mods);
        void _window_should_close_callback(GLFWwindow* w);
        void _window_size_callback(GLFWwindow* w, int width, int height);
        
        // receive events from windowing api
        // NpcDynamo will mirror the InputDynamo, but use internal logic instead of a window
        GLFWwindow* m_windowApi;
        // configurations for window api (since it is not abstracted to a class yet)
        int m_glfwMouseMode = GLFW_CURSOR_NORMAL;

        // store raw window api input for relays to reference
        RawInputBuffer m_inputBuffer;

        // Input Convertion Relays
        SpacialInputRelay m_spacialInputRelay;
    };
}

#endif // INPUT_DYNAMO_H