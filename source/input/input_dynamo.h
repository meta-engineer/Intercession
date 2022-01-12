#ifndef INPUT_DYNAMO_H
#define INPUT_DYNAMO_H

//#include "intercession_pch.h"
#include <GLFW/glfw3.h>

namespace pleep
{
    // input has to have feedback (unlike the render dynamo)
    // so its operation should be different, but the concept of a dynamo should be the same
    // an "engine" that is passed entities to "power" using the api
    // "input relays" can be subclassed to power specific outputs: 
    //      an entity, a component, or just a function callback
    class InputDynamo
    {
    public:
        InputDynamo(GLFWwindow* windowApi);
        ~InputDynamo();

        // poll event queue and process relays
        void update(double deltaTime);

        bool poll_close_signal();

    private:
        // recieve events from windowing api
        GLFWwindow* m_windowApi;

        // this is sus, could just directly use glfwWindowShouldClose for this
        // then its tied explicitly to windowing api
        bool m_closeSignal;
    };
}

#endif // INPUT_DYNAMO_H