#ifndef COSMOS_CONTEXT_H
#define COSMOS_CONTEXT_H

//#include "intercession_pch.h"

// external
// our "window api"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "cosmos.h"

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
        CosmosContext();
        CosmosContext(GLFWwindow* appWindow);
        ~CosmosContext();

        void run();
        void stop();

        // apis provide context for dynamos to use
        // should dynamo be contructed when a relevant api is attached?
        // or should an api reference be stored in the context.
        // Are apis and dynamos 1-to-1?

    private:
        //Cosmos* m_currentCosmos;

        GLFWwindow* m_appWindow;
        //RenderDynamo* m_renderDynamo;
        //InputDynamo* m_inputDynamo;
        //AudioDynamo* m_audioDynamo;
        //NetworkDynamo* m_netDynamo;

        bool m_running;
    };
}

#endif // COSMOS_CONTEXT_H