#include "input_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    InputDynamo::InputDynamo(std::shared_ptr<EventBroker> sharedBroker, GLFWwindow* windowApi) 
        : A_Dynamo(sharedBroker)
        , m_windowApi(windowApi)
        , m_spacialInputRelay(m_inputBuffer)
    {
        PLEEPLOG_TRACE("Start input pipeline setup");
        // we have "lease" of api to override callbacks
        // No other system can set the UserPointer (glfwSetWindowUserPointer) or this won't work
        _set_my_window_callbacks();

        // window config
        // set mouse capture mode
        glfwSetInputMode(m_windowApi, GLFW_CURSOR, m_glfwMouseMode);
        
        PLEEPLOG_TRACE("Done input pipeline setup");
    }
    
    InputDynamo::~InputDynamo() 
    {
    }
    
    void InputDynamo::submit(SpacialInputPacket data) 
    {
        // submit to relay
        m_spacialInputRelay.submit(data);
    }
    
    void InputDynamo::run_relays(double deltaTime) 
    {
        // this will call all registered callbacks
        glfwPollEvents();
        // Raw input buffer will be set now

        // engage relays with polled input
        m_spacialInputRelay.engage(deltaTime);

        // check for any interactions with window frame (not captured specifically by callbacks)
        // glfwWindowShouldClose will be set after glfwPollEvents
        if (glfwWindowShouldClose(m_windowApi))
        {
            // callback will reset shouldClose bit
            this->_window_should_close_callback(m_windowApi);
        }
    }
    
    void InputDynamo::reset_relays() 
    {
        m_spacialInputRelay.clear();
        
        // prepare buffer for next frame
        m_inputBuffer.flush();
    }
    
    void InputDynamo::_set_my_window_callbacks() 
    {
        // no one else better be expecting to use this...
        glfwSetWindowUserPointer(m_windowApi, this);
        
        // Link cursor callback to InputDynamo
        auto mouseMoveCallback = [](GLFWwindow* w, double x, double y)
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_mouse_move_callback(w, x, y);
        };
        glfwSetCursorPosCallback(m_windowApi, mouseMoveCallback);

        // Link scroll callback to InputDynamo
        auto mouseScrollCallback = [](GLFWwindow* w, double dx, double dy)
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_mouse_scroll_callback(w, dx, dy);
        };
        glfwSetScrollCallback(m_windowApi, mouseScrollCallback);
        
        // Link click callback to InputDynamo
        auto mouseClickCallback = [](GLFWwindow* w, int button, int action, int mods) 
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_mouse_button_callback(w, button, action, mods);
        };
        glfwSetMouseButtonCallback(m_windowApi, mouseClickCallback);

        // Link key event callback to InputDynamo
        auto keyCallback = [](GLFWwindow* w, int key, int scancode, int action, int mods)
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_key_callback(w, key, scancode, action, mods);
        };
        glfwSetKeyCallback(m_windowApi, keyCallback);

        // Framebuffer resize needs to change glViewport and resize RenderDynamo FBOs
        // so it should defer to RenderDynamo to deal with those...
        auto windowSizeCallback = [](GLFWwindow* w, int width, int height)
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_window_size_callback(w, width, height);
        };
        glfwSetWindowSizeCallback(m_windowApi, windowSizeCallback);

        // WindowShouldClose should be reported to cosmos (via synchro) or the parent context
        auto windowCloseCallback = [](GLFWwindow* w)
        {
            static_cast<InputDynamo*>(glfwGetWindowUserPointer(w))->_window_should_close_callback(w);
        };
        glfwSetWindowCloseCallback(m_windowApi, windowCloseCallback);
    }
    
    void InputDynamo::_mouse_move_callback(GLFWwindow* w, double x, double y) 
    {
        // silence warning while not using
        UNREFERENCED_PARAMETER(w);
        //PLEEPLOG_DEBUG("mouse: " + std::to_string(x) + ", " + std::to_string(y));
        
        static double lastX = -1;
        static double lastY = -1;

        if (m_glfwMouseMode == GLFW_CURSOR_DISABLED)
        {
            // if focused provide mouse as analog input
            // we'll assign the mouse to be analog 0
            m_inputBuffer.setTwoDimAnalog(
                0,
                lastX == -1.0 ? 0.0 : (x - lastX),
                lastY == -1.0 ? 0.0 : (y - lastY)
            );
        }
        else
        {
            // otherwise provide absolute cursor position
            m_inputBuffer.setScreenPosition(x, y);
        }

        lastX = x;
        lastY = y;
    }
    
    void InputDynamo::_mouse_scroll_callback(GLFWwindow* w, double dx, double dy) 
    {
        // silence warning while not using
        UNREFERENCED_PARAMETER(w);

        // only provide scroll if mouse is focused
        if (m_glfwMouseMode != GLFW_CURSOR_DISABLED)
        {
            return;
        }
        
        // we'll assign the scroll wheel to be analog 1
        m_inputBuffer.setTwoDimAnalog(1, dx, dy);

        //PLEEPLOG_TRACE("Mouse scroll callback. Scrolled (" + std::to_string(dx) + ", " + std::to_string(dy) + ")");
    }
    
    void InputDynamo::_mouse_button_callback(GLFWwindow* w, int button, int action, int mods) 
    {
        // silence warning while not using
        UNREFERENCED_PARAMETER(w);
        // NOTE: mods is bitfield of key mods (shift, control, alt, super, caps, numlock)
        // https://www.glfw.org/docs/3.3/group__mods.html
        UNREFERENCED_PARAMETER(mods);

        // only provide clicks if mouse is focused
        if (m_glfwMouseMode != GLFW_CURSOR_DISABLED)
        {
            return;
        }

        // action is one of GLFW_PRESS || GLFW_RELEASE
        // buttons should be in range [0,8]
        m_inputBuffer.setDigital(button, action);

        // mods will be set on key callback for relays to use
    }
    
    void InputDynamo::_key_callback(GLFWwindow* w, int key, int scancode, int action, int mods) 
    {
        // silence warning while not using
        UNREFERENCED_PARAMETER(w);
        // NOTE: mods is bitfield of key mods (shift, control, alt, super, caps, numlock)
        // https://www.glfw.org/docs/3.3/group__mods.html
        UNREFERENCED_PARAMETER(mods);
        // scancode changes on physical keyboard layout
        UNREFERENCED_PARAMETER(scancode);
        
        // we'll probably NEVER want to use keyboard auto-repeat (maybe for typing?)
        if (action == GLFW_REPEAT) return;
        
        //PLEEPLOG_TRACE("Key event: " + std::to_string(key) + ", code: " + std::to_string(scancode) + ", " + std::to_string(action));

        m_inputBuffer.setDigital(key, action);

        // disambiguate left and right shift/alt/etc...
        m_inputBuffer.convertMods(mods);

        // ****** HARDCODED SHORTCUTS ******
        if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
        {
            m_glfwMouseMode = m_glfwMouseMode == GLFW_CURSOR_NORMAL ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
            glfwSetInputMode(m_windowApi, GLFW_CURSOR, m_glfwMouseMode);
        }
    }
    
    void InputDynamo::_window_should_close_callback(GLFWwindow* w) 
    {
        UNREFERENCED_PARAMETER(w);

        // always clear bit?
        glfwSetWindowShouldClose(w, GLFW_FALSE);

        // Cosmos that I am powering could use this info...
        // ill send window::QUIT event, cosmos should pick this up and 
        // foreward as a cosmos::QUIT for context
        // temporary: context will handle this and stop main loop
        PLEEPLOG_TRACE("Sending event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");
        m_sharedBroker->send_event(events::window::QUIT);
    }
    
    void InputDynamo::_window_size_callback(GLFWwindow* w, int width, int height) 
    {
        UNREFERENCED_PARAMETER(w);
        
        PLEEPLOG_TRACE("Sending event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) {" + std::to_string(width) + ", " + std::to_string(height) + "}");

        // Send event to Render Dynamo
        EventMessage resizeEvent(events::window::RESIZE);
        events::window::RESIZE_params resizeParams { width, height };
        resizeEvent << resizeParams;

        m_sharedBroker->send_event(resizeEvent);
    }
}