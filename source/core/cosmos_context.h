#ifndef COSMOS_CONTEXT_H
#define COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include "cosmos.h"

// external
#include <GLFW/glfw3.h>

namespace pleep
{
    // maintains the interactive "world"/"level" to update
    // maintain the dynamos and their resources (render, audio, networking)
    // maintains a current cosmos and attaches dynamos to it
    // runs main game loop, update cosmos
    class CosmosContext
    {
    public:
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