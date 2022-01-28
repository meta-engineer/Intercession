#ifndef COSMOS_CONTEXT_H
#define COSMOS_CONTEXT_H

//#include "intercession_pch.h"

// external
#include <memory>
// our "window api"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/cosmos.h"
#include "events/event_broker.h"
#include "rendering/render_dynamo.h"
#include "controlling/control_dynamo.h"

namespace pleep
{
    // maintain the dynamos and their api resources (render, audio, networking)
    // maintains a current interactive "world" aka cosmos and attaches dynamos to it
    // manages loading cosmoses and handling transitioning between different ones
    // runs main game loop
    class CosmosContext
    {
    public:
        // I need to know some kind of configuration to know what cosmos to build first,
        //   dynamos/synchros I can expect, and a state machine for handling cosmos transition events
        // maybe some "multiverse" file which is a manifest for "scene"/"cosmos" files

        // I doubt I'll want to construct a context without ANY apis, so this could be changed
        //   into an _init() method called once by other constructors
        CosmosContext();
        // accept all apis that a Cosmos could use
        // apis provide system resources for dynamos
        // on construction we will build all the dynamos from these apis 
        //   that will last for Context lifetime 
        //   (no dynamically attaching apis afterward)
        CosmosContext(GLFWwindow* windowApi);   // etc...
        ~CosmosContext();

        // main loop
        void run();
        // stop main loop, Context should handle an Event which calls this
        void stop();

    private:
        // populate the cosmos
        // for now this has no parameters (scene filename in future?)
        // provide registered synchros with our dynamos
        void _build_cosmos();

        // Listening to events:window::QUIT sent by ControlDynamo
        void _quit_handler(Event quitEvent);

        Cosmos* m_currentCosmos;

        // shared event distributor (pub/sub) to be used by context (me), my dynamos, and synchros that attach to those dynamos
        EventBroker* m_eventBroker;

        // dynamos store relevant api references passed here from AppGateway
        // therefore dynamos must live for the context's lifetime, we cannot rebuild them
        // created cosmos' share dynamos with their synchros
        RenderDynamo* m_renderDynamo;
        ControlDynamo* m_controlDynamo;

        //AudioApi* m_audioApi;
        //AudioDynamo* m_audioDynamo;

        //NetworkApi* m_netApi;
        //NetworkDynamo* m_netDynamo;

        bool m_running;
    };
}

#endif // COSMOS_CONTEXT_H