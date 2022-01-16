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
#include "rendering/render_dynamo.h"
#include "controlling/control_dynamo.h"

namespace pleep
{
    // manages change between interactive "worlds" aka cosmoses
    // maintain the dynamos and their resources (render, audio, networking)
    // maintains a current cosmos and attaches dynamos to it
    // runs main game loop, update cosmos
    class CosmosContext
    {
    public:
        // I need to know some kind of configuration to know what cosmos to build first + scene information, and a state machine for comoses to change to/from
        // or some "multiverse" file which lists where to look for "scene" files
        // use scene files to build cosmos
        CosmosContext();
        CosmosContext(GLFWwindow* windowApi);   // etc...
        ~CosmosContext();

        // main loop, exits when context dies
        void run();
        // respond to Cosmos update and stop main loop
        void stop();

        // apis provide context for dynamos to use
        // should dynamo be contructed when a relevant api is attached?
        // or should an api reference be stored in the context.
        // Are apis and dynamos 1-to-1?

    private:
        Cosmos* m_currentCosmos;

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