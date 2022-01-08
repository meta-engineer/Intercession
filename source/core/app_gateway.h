#ifndef APP_GATEWAY_H
#define APP_GATEWAY_H

// std
#include <memory>
#include <iostream>
#include <cassert>

// external
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//internal
#include "logging/pleep_log.h"
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
        void _build_window_api();
        void _clean_window_api();

        //void _build_audio_api();
        //void _build_network_api();

        // on context build we pass all current apis
        // if an api hasn't been constructed the context will recieve a nullptr
        void _build_context();
        // we might have to check and wait for cxt thread to join.
        void _clean_context();

        bool m_running;

    private:
        // should have a window abstraction layer
        // hard-config to use glfw for now...
        GLFWwindow* m_glfwWindow;

        //AudioApi* m_audioApi;
        //NetApi*   m_netApi;

        // TODO: smart pointers
        CosmosContext* m_ctx;
    };
}

#endif // APP_GATEWAY_H