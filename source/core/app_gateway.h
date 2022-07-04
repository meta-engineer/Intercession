#ifndef APP_GATEWAY_H
#define APP_GATEWAY_H

// std
#include <memory>
#include <cassert>

// external
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//internal
#include "cosmos_context.h"

// these should be defined in some use config file and part of a window abstraction layer
#define DEFAULT_WINDOW_WIDTH 1290
#define DEFAULT_WINDOW_HEIGHT 540
#define DEFAULT_WINDOW_TITLE "Intercession"
#define CONFIG_DIRECTORY "config/"  //starts in pwd

namespace pleep
{
    // Abstract base class for the root "app"
    // maintains apis for external resources (windowing, audio, networking, file system)
    // provides CosmosContext (constructs with) access to these resources
    class AppGateway
    {
    protected:
        AppGateway();
    public:
        ~AppGateway();

    public:
        void run();
        void stop();
        void reset();

    protected:
        // Create/configure all api resources, and then construct m_ctx using them
        virtual void _build_gateway() {};
        virtual void _clean_gateway() {};

        // For now we'll assume apis are static and won't change after the Gateway is constructed
        // they will be build by subclass _build_gateway and directly given to a context on construction
        // context must be built AFTER api
        // cleaning apis without cleaning context first (or informing it) has undefined behaviour
        void _build_window_api();
        void _clean_window_api();
        //void _build_audio_api();
        //void _build_network_api();


        bool m_running;

        // should have a window abstraction layer, hard-code to use glfw for now...
        // are smart pointers better? I only want this class to have "ownership"
        // and only want this class to manage the memory
        // Ideally all other references would be nulled (stopping undefined behaviour) if I delete it.
        GLFWwindow* m_windowApi;
        //AudioApi* m_audioApi;
        //NetApi*   m_netApi;

        // Subclasses construct context with pointers to apis, giving them a "lease"
        // (Only AppGateway manages memory)
        // Only 1 context should exist per gateway so there aren't "race conditions" between two contexts (singleton?)
        CosmosContext* m_ctx;
    };
}

#endif // APP_GATEWAY_H