#ifndef CONTROL_DYNAMO_H
#define CONTROL_DYNAMO_H

//#include "intercession_pch.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/a_dynamo.h"
#include "controlling/camera_control_packet.h"
#include "controlling/physics_control_packet.h"
#include "controlling/biped_control_packet.h"
#include "controlling/fly_control_relay.h"
#include "controlling/sotc_camera_control_relay.h"
#include "controlling/basic_biped_control_relay.h"
#include "controlling/biped3p_camera_control_relay.h"
#include "controlling/spacial_input_buffer.h"

namespace pleep
{
    // input has to have feedback (unlike the render/other dynamos)
    // so its operation should be different, but the concept of a dynamo should be the same
    // an "engine" that is passed entities to "power" using the api
    // relays will uniquely get a pointer to the ecs so it can dynamically pick components

    // "control relays" can be subclassed to power specific components
    // a "controller" component is a semantic link (like a material) for input synchro
    // materials know render relays, controllers know control relays

    // Dynamo ingests raw input data from api, on run it translates
    //   and stores it into "action buffers" each frame
    // All relays owned by dynamo will have reference to action buffer(s)
    //   which it will use to determine operation
    class ControlDynamo : public A_Dynamo
    {
    public:
        ControlDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi);
        ~ControlDynamo();

        // should control components be "submitted" or statically "registered"
        // "immediate mode" vs "retained mode"
        void submit(CameraControlPacket data);
        void submit(PhysicsControlPacket data);
        void submit(BipedControlPacket data);

        // poll event queue and process relays
        // THROWS runtime_error if m_windowApi is null
        // Dynamo translates raw input from callbacks to application specific "actions" for relays
        void run_relays(double deltaTime) override;

        // prepare relays for next frame
        void reset_relays() override;

    private:
        // signal to m_sharedBroker that an entity has been modified
        void _signal_modified_entity(Entity id);

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

        // collection of input information from this frame
        // this is an abstraction of the raw api input that my relays will have references to
        SpacialInputBuffer m_spacialInputBuffer;

        int m_glfwMouseMode = GLFW_CURSOR_NORMAL;

        // Specific Relays
        // TODO: this should probably be in a specific ControlDynamo subclass built by the context
        // TODO: I should source these from some sort of relay library
        std::unique_ptr<FlyControlRelay> m_flyController;
        std::unique_ptr<SotcCameraControlRelay> m_cameraController;
        std::unique_ptr<BasicBipedControlRelay> m_bipedController;
        std::unique_ptr<Biped3pCameraControlRelay> m_bipedCameraController;
    };
}

#endif // CONTROL_DYNAMO_H