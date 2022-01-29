#include "control_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    ControlDynamo::ControlDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi)
        : IDynamo(sharedBroker)
        , m_windowApi(windowApi)
    {
        // we have "lease" of api to override callbacks
        _set_my_window_callbacks();

        // initalize control related window config
        // set mouse capture mode
        //glfwSetInputMode(m_windowApi, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    ControlDynamo::~ControlDynamo()
    {
    }
    
    void ControlDynamo::prime() 
    {
        // fatal error if window api has been destroyed
        if (m_windowApi == nullptr)
        {
            PLEEPLOG_ERROR("Window API reference was unexpectedly NULL");
            throw std::runtime_error("ControlDynamo update found window api reference unexpectedly null");
        }
    }
    
    void ControlDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        // this will call all triggered callbacks
        glfwPollEvents();

        // update each relay inside callbacks
        // relays could "subcribe" to an input?
        // the relay will have to be pre-attached to an entity component which it can modify.
        // return false if a relay fails

        //glfwWindowShouldClose will be set after glfwPollEvents
        if (glfwWindowShouldClose(m_windowApi))
        {
            // callback will reset shouldClose bit
            this->_window_should_close_callback(m_windowApi);
        }
    }

    void ControlDynamo::_set_my_window_callbacks()
    {
        // no one else better be expecting to use this...
        glfwSetWindowUserPointer(m_windowApi, this);

        // Link cursor callback to ControlDynamo
        auto mouseMoveCallback = [](GLFWwindow* w, double x, double y)
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_mouse_move_callback(w, x, y);
        };
        glfwSetCursorPosCallback(m_windowApi, mouseMoveCallback);

        // Link scroll callback to ControlDynamo
        auto mouseScrollCallback = [](GLFWwindow* w, double dx, double dy)
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_mouse_scroll_callback(w, dx, dy);
        };
        glfwSetScrollCallback(m_windowApi, mouseScrollCallback);
        
        // Link click callback to ControlDynamo
        auto mouseClickCallback = [](GLFWwindow* w, int button, int action, int mods) 
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_mouse_button_callback(w, button, action, mods);
        };
        glfwSetMouseButtonCallback(m_windowApi, mouseClickCallback);

        // Link key event callback to ControlDynamo
        auto keyCallback = [](GLFWwindow* w, int key, int scancode, int action, int mods)
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_key_callback(w, key, scancode, action, mods);
        };
        glfwSetKeyCallback(m_windowApi, keyCallback);

        // Framebuffer resize needs to change glViewport and resize RenderDynamo FBOs
        // so it should defer to RenderDynamo to deal with those...
        auto windowSizeCallback = [](GLFWwindow* w, int width, int height)
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_window_size_callback(w, width, height);
        };
        glfwSetWindowSizeCallback(m_windowApi, windowSizeCallback);

        // WindowShouldClose should be reported to cosmos (via synchro) or the parent context
        auto windowCloseCallback = [](GLFWwindow* w)
        {
            static_cast<ControlDynamo*>(glfwGetWindowUserPointer(w))->_window_should_close_callback(w);
        };
        glfwSetWindowCloseCallback(m_windowApi, windowCloseCallback);
    }
    
    void ControlDynamo::_mouse_move_callback(GLFWwindow* w, double x, double y) 
    {
        // silence warning while not using
        static_cast<void*>(w);

        static double lastX = -1;
        static double lastY = -1;

        //double xoffset = lastX == -1.0 ? 0.0 : (x - lastX);
        //double yoffset = lastY == -1.0 ? 0.0 : (lastY - y);

        lastX = x;
        lastY = y;

        //PLEEPLOG_TRACE("Mouse move callback. Moved (" + std::to_string(xoffset) + ", " + std::to_string(yoffset) + ")");
    }
    
    void ControlDynamo::_mouse_scroll_callback(GLFWwindow* w, double dx, double dy) 
    {
        // silence warning while not using
        static_cast<void*>(w);
        static_cast<void>(dx);
        static_cast<void>(dy);

        //cm.set_view_fov(cm.get_view_fov() - (float)dy);
        PLEEPLOG_TRACE("Mouse scroll callback. Scrolled (" + std::to_string(dx) + ", " + std::to_string(dy) + ")");
    }
    
    void ControlDynamo::_mouse_button_callback(GLFWwindow* w, int button, int action, int mods) 
    {
        UNREFERENCED_PARAMETER(w);
        // NOTE: mods is key mods (shift, control alt, super, caps, numlock)
        UNREFERENCED_PARAMETER(mods);

        if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Left mouse press!");
        }
        if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Right mouse press!");
        }
        if (button == GLFW_MOUSE_BUTTON_3 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Middle mouse press!");
        }
        if (button == GLFW_MOUSE_BUTTON_4 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Mouse button 4 press!");
        }
        if (button == GLFW_MOUSE_BUTTON_5 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Mouse button 5 press!");
        }
        if (button == GLFW_MOUSE_BUTTON_6 && action == GLFW_PRESS)
        {
            PLEEPLOG_TRACE("Mouse button 6 press!");
        }
    }
    
    void ControlDynamo::_key_callback(GLFWwindow * w, int key, int scancode, int action, int mods) 
    {
        UNREFERENCED_PARAMETER(w);
        UNREFERENCED_PARAMETER(scancode);
        UNREFERENCED_PARAMETER(mods);
        PLEEPLOG_TRACE("Key event: " + std::to_string(key) + ", code: " + std::to_string(scancode));

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            this->_window_should_close_callback(w);
        }
    }
    
    void ControlDynamo::_window_should_close_callback(GLFWwindow* w) 
    {
        UNREFERENCED_PARAMETER(w);

        // always clear bit?
        glfwSetWindowShouldClose(w, GLFW_FALSE);

        // Cosmos that I am powering could use this info...
        // ill send window::QUIT event, cosmos should pick this up and 
        // foreward as a cosmos::QUIT for context
        PLEEPLOG_TRACE("Sending event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");
        m_sharedBroker->send_event(events::window::QUIT);
    }
    
    void ControlDynamo::_window_size_callback(GLFWwindow* w, int width, int height) 
    {
        UNREFERENCED_PARAMETER(w);
        
        PLEEPLOG_TRACE("Sending event " + std::to_string(events::window::RESIZE) + " (events::window::RESIZE) {" + std::to_string(width) + ", " + std::to_string(height) + "}");

        // Send event to Render Dynamo
        // TODO: send resize event with params
        Event resizeEvent(events::window::RESIZE);
        events::window::resize::Params resizeParams { width, height };
        resizeEvent.set_param(resizeParams);

        m_sharedBroker->send_event(resizeEvent);
    }
    
}