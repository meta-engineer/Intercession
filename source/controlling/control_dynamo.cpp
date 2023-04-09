#include "control_dynamo.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    ControlDynamo::ControlDynamo(EventBroker* sharedBroker, GLFWwindow* windowApi)
        : A_Dynamo(sharedBroker)
        , m_windowApi(windowApi)
        // setup relays with access to my input buffers (we need to ecplicitly know which buffers relay needs)
        , m_flyController(std::make_unique<FlyControlRelay>(m_spacialInputBuffer))
        , m_cameraController(std::make_unique<SotcCameraControlRelay>(m_spacialInputBuffer))
        , m_bipedController(std::make_unique<BasicBipedControlRelay>(m_spacialInputBuffer))
        , m_bipedCameraController(std::make_unique<Biped3pCameraControlRelay>(m_spacialInputBuffer))
    {
        PLEEPLOG_TRACE("Setup Control pipeline");
        // we have "lease" of api to override callbacks
        _set_my_window_callbacks();

        // initalize control related window config
        // set mouse capture mode
        glfwSetInputMode(m_windowApi, GLFW_CURSOR, m_glfwMouseMode);
        
        PLEEPLOG_TRACE("Done Control pipeline setup");
    }
    
    ControlDynamo::~ControlDynamo()
    {
    }
    
    void ControlDynamo::submit(CameraControlPacket data)
    {
        // dispatch packet to designated relay
        switch (data.controller.type)
        {
            case CameraControlType::sotc:
                m_cameraController->submit(data);
                break;
            case CameraControlType::biped3p:
                m_bipedCameraController->submit(data);
                break;
            default:
                PLEEPLOG_WARN("Dynamo has no handler for Camera Controller type: " + std::to_string(data.controller.type) + ". Default behaviour is to ignore.");
                return;
        }

        // camera doesn't need to notify its updates to network?
        //this->_signal_modified_entity(data.controllee);
    }

    void ControlDynamo::submit(PhysicsControlPacket data)
    {
        // dispatch packet to designated relay
        switch (data.controller.type)
        {
            case PhysicsControlType::position:
                m_flyController->submit(data);
                break;
            default:
                PLEEPLOG_WARN("Dynamo has no handler for Physics Controller type: " + std::to_string(data.controller.type) + ". Default behaviour is to ignore.");
                return;
        }
        
        this->_signal_modified_entity(data.controllee);
    }
    
    void ControlDynamo::submit(BipedControlPacket data)
    {
        // no subtype for biped components, so we'll just dispatch to our known capable relay
        m_bipedController->submit(data);
        
        this->_signal_modified_entity(data.controllee);
    }

    void ControlDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        // this will call all triggered callbacks
        glfwPollEvents();

        // callbacks will have translated raw input into input buffers now

        // Only send SpacialActions with locked mouse
        if (m_glfwMouseMode != GLFW_CURSOR_DISABLED)
        {
            m_spacialInputBuffer.clear();
        }

        m_flyController->engage(deltaTime);
        m_cameraController->engage(deltaTime);
        m_bipedController->engage(deltaTime);
        m_bipedCameraController->engage(deltaTime);

        // prepare input for next frame
            // key members should be cleared only on "release" action
        m_spacialInputBuffer.resolve();

        // check for interactions with window frame (not captured specifically by callbacks)
        // glfwWindowShouldClose will be set after glfwPollEvents
        if (glfwWindowShouldClose(m_windowApi))
        {
            // callback will reset shouldClose bit
            this->_window_should_close_callback(m_windowApi);
        }
    }

    void ControlDynamo::reset_relays()
    {
        // after 1+ timesteps clear relays
        m_flyController->clear();
        m_cameraController->clear();
        m_bipedController->clear();
        m_bipedCameraController->clear();
    }
    
    void ControlDynamo::_signal_modified_entity(Entity entity) 
    {
        EventMessage entityModified(events::cosmos::ENTITY_MODIFIED);
        events::cosmos::ENTITY_MODIFIED_params entityModifiedData;
        entityModifiedData.entity = entity;
        entityModified << entityModifiedData;
        m_sharedBroker->send_event(entityModified);
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

        double xoffset = lastX == -1.0 ? 0.0 : (x - lastX);
        double yoffset = lastY == -1.0 ? 0.0 : (lastY - y);

        lastX = x;
        lastY = y;

        //PLEEPLOG_TRACE("Mouse move callback. Moved (" + std::to_string(xoffset) + ", " + std::to_string(yoffset) + ")");
        
        if (yoffset > 0)
        {
            m_spacialInputBuffer.set(SpacialActions::rotatePitchUp, true, (float)abs(yoffset));
        }
        else
        {
            m_spacialInputBuffer.set(SpacialActions::rotatePitchDown, true, (float)abs(yoffset));
        }
        
        if (xoffset > 0)
        {
            m_spacialInputBuffer.set(SpacialActions::rotateYawRight, true, (float)abs(xoffset));
        }
        else
        {
            m_spacialInputBuffer.set(SpacialActions::rotateYawLeft, true, (float)abs(xoffset));
        }
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

        // action is one of GLFW_PRESS || GLFW_RELEASE
        // Relay will have to track when press starts vs held

        if (button == GLFW_MOUSE_BUTTON_1)
        {
            //PLEEPLOG_TRACE("Left mouse press!");
            m_spacialInputBuffer.set(SpacialActions::action0, action,  1);
        }
        if (button == GLFW_MOUSE_BUTTON_2)
        {
            //PLEEPLOG_TRACE("Right mouse press!");
            m_spacialInputBuffer.set(SpacialActions::action1, action,  1);
        }
        if (button == GLFW_MOUSE_BUTTON_3)
        {
            //PLEEPLOG_TRACE("Middle mouse press!");
            m_spacialInputBuffer.set(SpacialActions::action2, action,  1);
        }
        if (button == GLFW_MOUSE_BUTTON_4)
        {
            //PLEEPLOG_TRACE("Mouse button 4 press!");
            m_spacialInputBuffer.set(SpacialActions::action3, action,  1);
        }
        if (button == GLFW_MOUSE_BUTTON_5)
        {
            //PLEEPLOG_TRACE("Mouse button 5 press!");
            m_spacialInputBuffer.set(SpacialActions::action4, action,  1);
        }
        if (button == GLFW_MOUSE_BUTTON_6)
        {
            //PLEEPLOG_TRACE("Mouse button 6 press!");
            m_spacialInputBuffer.set(SpacialActions::action5, action,  1);
        }
    }
    
    void ControlDynamo::_key_callback(GLFWwindow * w, int key, int scancode, int action, int mods) 
    {
        UNREFERENCED_PARAMETER(w);
        UNREFERENCED_PARAMETER(scancode);
        UNREFERENCED_PARAMETER(mods);
        //PLEEPLOG_TRACE("Key event: " + std::to_string(key) + ", code: " + std::to_string(scancode) + ", " + std::to_string(action));

        // we'll probably NEVER want to use keyboard auto-repeat (maybe for typing?)
        if (action == GLFW_REPEAT) return;

        // Hardcoded actions
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            this->_window_should_close_callback(w);
        }
        if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
        {
            if (m_glfwMouseMode != GLFW_CURSOR_NORMAL)
            {
                m_glfwMouseMode = GLFW_CURSOR_NORMAL;
            }
            else
            {
                m_glfwMouseMode = GLFW_CURSOR_DISABLED;
            }
            glfwSetInputMode(m_windowApi, GLFW_CURSOR, m_glfwMouseMode);
        }

        // fill input buffer actions
        // dynamo specifies translation of api (devices) to input buffer
        // format of input buffer must be synchronized with relays
        if (key == GLFW_KEY_W)
        {
            m_spacialInputBuffer.set(SpacialActions::moveForward, action,  1);
        }
        if (key == GLFW_KEY_S)
        {
            m_spacialInputBuffer.set(SpacialActions::moveBackward, action, 1);
        }
        if (key == GLFW_KEY_D)
        {
            m_spacialInputBuffer.set(SpacialActions::moveLeft, action,  1);
        }
        if (key == GLFW_KEY_A)
        {
            m_spacialInputBuffer.set(SpacialActions::moveRight, action, 1);
        }
        if (key == GLFW_KEY_SPACE)
        {
            m_spacialInputBuffer.set(SpacialActions::moveUp, action,  1);
        }
        if (key == GLFW_KEY_C)
        {
            m_spacialInputBuffer.set(SpacialActions::moveDown, action, 1);
        }
/*
        if (key == GLFW_KEY_UP)
        {
            m_spacialInputBuffer.set(SpacialActions::rotatePitchUp, action, 1);
        }
        if (key == GLFW_KEY_DOWN)
        {
            m_spacialInputBuffer.set(SpacialActions::rotatePitchDown, action, 1);
        }
        if (key == GLFW_KEY_LEFT)
        {
            m_spacialInputBuffer.set(SpacialActions::rotateYawLeft, action, 1);
        }
        if (key == GLFW_KEY_RIGHT)
        {
            m_spacialInputBuffer.set(SpacialActions::rotateYawRight, action, 1);
        }
*/
    }
    
    void ControlDynamo::_window_should_close_callback(GLFWwindow* w) 
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
    
    void ControlDynamo::_window_size_callback(GLFWwindow* w, int width, int height) 
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