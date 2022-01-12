#ifndef APP_GATEWAY_H
#define APP_GATEWAY_H

// std
#include <memory>
#include <iostream>
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

namespace pleep
{
    // root app class
    // maintains apis for external resources (windowing, audio, networking, file system)
    // provides CosmosContext access to these resources
    class AppGateway
    {
    public:
        AppGateway();
        ~AppGateway();

        void run();
        void stop();
        void reset();

    private:
        // For now we'll assume apis are static and won't change after the Gateway is constructed
        // they will be directly given to a context on construction
        // building an api after building context will not attach it, context must be rebuilt
        void _build_window_api();
        void _clean_window_api();

        //void _build_audio_api();
        //void _build_network_api();

        // on context build we give "lease" of all current apis' to context
        // (only 1 context per gateway should exist at a time)
        // if an api hasn't been constructed the context will recieve a nullptr for that api
        void _build_context();
        // we might have to check and wait for cxt thread to join.
        void _clean_context();

        bool m_running;

    private:
        // should have a window abstraction layer
        // hard-code to use glfw for now...
        // are smart pointers better? I only want this class to have "ownership"
        // I only want this class to manage the emeory, 
        // and I want all other references to be nulled (stopping undefined behaviour) if I delete it.
        GLFWwindow* m_windowApi;

        //AudioApi* m_audioApi;
        //NetApi*   m_netApi;

        CosmosContext* m_ctx;
    };
}

#endif // APP_GATEWAY_H