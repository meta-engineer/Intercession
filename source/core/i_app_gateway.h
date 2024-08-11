#ifndef I_APP_GATEWAY_H
#define I_APP_GATEWAY_H

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
#include "core/i_cosmos_context.h"

// these should be defined in some use config file and part of a window abstraction layer
#define DEFAULT_WINDOW_WIDTH 1290
#define DEFAULT_WINDOW_HEIGHT 540
#define XSTR(s) STR(s)
#define STR(s) #s
#define DEFAULT_WINDOW_TITLE "Intercession v" XSTR(BUILD_VERSION_MAJOR) "." XSTR(BUILD_VERSION_MINOR) "." XSTR(BUILD_VERSION_PATCH)
#define CONFIG_DIRECTORY "config/"  //starts in pwd

namespace pleep
{
    // Abstract base class for the root "app"
    // maintains apis for external resources (windowing, audio, networking, file system)
    // creates CosmosContext(s) (on construction) and provides access to these resources
    class I_AppGateway
    {
    protected:
        // Create/configure all api resources, and then construct context(s) using them
        I_AppGateway() = default;
    public:
        virtual ~I_AppGateway() = default;

    public:
        // AppGateway is the top-most level of the app so it does not internally stop or restart
        // run is called once at app start and only returns once the app has reached end of lifetime
        virtual void run() = 0;

    protected:
        // TODO: move apis & methods to the Gateway subclasses that need them (or there any overlap between client/server?) and keep this as a strict interface

        // For now we'll assume apis are static and won't change after the Gateway is constructed
        // they will be build by subclass _build_gateway and directly given to a context on construction
        // context must be built AFTER api
        // cleaning apis without cleaning context first (or informing it) has undefined behaviour
        void _build_window_api();
        void _clean_window_api();
        //void _build_audio_api();
        //void _build_network_api();


        // should have a window abstraction layer, hard-code to use glfw for now...
        // are smart pointers better? I only want this class to have "ownership"
        // and only want this class to manage the memory
        // Ideally all other references would be nulled (stopping undefined behaviour) if I delete it.
        GLFWwindow* m_windowApi = nullptr;
        //AudioApi* m_audioApi;
        //NetApi*   m_netApi;

        // Subclasses construct context(s) and pass in apis, giving them a "lease"
        // (aka only AppGateway manages memory)
    };
}

#endif // I_APP_GATEWAY_H