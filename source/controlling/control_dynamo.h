#ifndef CONTROL_DYNAMO_H
#define CONTROL_DYNAMO_H

//#include "intercession_pch.h"
#include <GLFW/glfw3.h>

namespace pleep
{
    // input has to have feedback (unlike the render dynamo)
    // so its operation should be different, but the concept of a dynamo should be the same
    // an "engine" that is passed entities to "power" using the api
    // "control relays" could be subclassed to power specific components
    // a "controller" component might be a good semantic link (like a material) for input synchro
    // some sort of priority system can be used to shift output targets of the dynamo
    // materials know render relays, controllers know control relays?
    class ControlDynamo
    {
    public:
        ControlDynamo(GLFWwindow* windowApi);
        ~ControlDynamo();

        // poll event queue and process relays
        void update(double deltaTime);

        bool poll_close_signal();


    private:
        // recieve events from windowing api
        GLFWwindow* m_windowApi;
        
        // this is sus, could just directly use glfwWindowShouldClose for this
        // then its tied explicitly to windowing api
        // however, we do want a chance to intercept window close "requests" (like a confirm message)
        bool m_closeSignal;     
    };
}

#endif // CONTROL_DYNAMO_H