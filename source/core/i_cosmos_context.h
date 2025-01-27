#ifndef I_COSMOS_CONTEXT_H
#define I_COSMOS_CONTEXT_H

//#include "intercession_pch.h"

// external
#include <memory>
#include <chrono>

// our "window api"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>

#include "core/cosmos.h"
#include "events/event_broker.h"
#include "core/dynamo_cluster.h"

namespace pleep
{
    const uint16_t FRAMERATE = 45;

    // Abstract base class that:
    // maintains a current interactive "world" aka cosmos and attaches dynamos to it
    // manages loading cosmoses and handling transitioning between different ones
    // runs main game loop
    // subclasses should maintain the dynamos and their api resources (render, audio, networking) to be used by the cosmos
    class I_CosmosContext
    {
        // I need to know some kind of configuration to know what cosmos to build first,
        //   dynamos/synchros I can expect, and a state machine for handling cosmos transition events
        // maybe some "multiverse" file which is a manifest for "scene"/"cosmos" files

    protected:
        // Setup shared resources (event broker)
        I_CosmosContext();
    public:
        virtual ~I_CosmosContext();

    public:
        // main loop
        void run();
        // stop main loop, Context should handle an Event which calls this
        void stop();
        // check run state (for when run is called on a different thread)
        bool is_running() const;

        // starts internal thread inside of run()
        void start();
        // waits until internal thread finishes, joins, and returns true
        // if internal thread is not joinable, returns false
        bool join();
        // returns if internal thread is joinable
        bool joinable();

    protected:
        // Runtime pipeline:
        // 1. m_currentCosmos updates
        // 2. _on_fixed is called as many times as will modulo into the time since last frame
        // 3. _on_frame is called once with time since last frame
        // 4. _clean_frame is called once (context is responsible for notifying dynamos when they should clear their packets for next frame)
        // Pipeline methods not pure virtual because you MAY not want to implement a particular step

        // called to setup dynamo relays with packets from synchros
        virtual void _prime_frame() {};
        // called by run with static timestep value
        virtual void _on_fixed(double fixedTime) { UNREFERENCED_PARAMETER(fixedTime); }
        // called by run() as often as possible with time since last call
        virtual void _on_frame(double deltaTime) { UNREFERENCED_PARAMETER(deltaTime); }
        // cleanup before next cosmos update
        virtual void _clean_frame() {}

        // Listening to events:window::QUIT sent by InputDynamo
        void _quit_handler(EventMessage quitEvent);

        // subclasses should create this as they see fit
        std::shared_ptr<Cosmos> m_currentCosmos = nullptr;
        // context owns its own thread to call run() on
        std::thread m_cosmosThread;
        bool m_isRunning = false;

        // shared event distributor (pub/sub) to be used by context (me), my dynamos, and synchros that attach to those dynamos
        std::shared_ptr<EventBroker> m_eventBroker;
        // Subclasses should instantiate whichever dynamos as they see fit
        // Our cosmos shares these dynamos with their synchros
        DynamoCluster m_dynamoCluster;

        // runtime calibrations
        // Fixed timestep for input processing
        const std::chrono::duration<double> m_fixedTimeStep = 
            std::chrono::duration<double>(1.0/FRAMERATE);
        // mechanism for tracking how many fixed timesteps to process
        std::chrono::duration<double> m_fixedTimeRemaining = 
            std::chrono::duration<double>(0.0);
        // set HARD frame max something reasonable to prevent squealing
        const std::chrono::duration<double> m_minFrameTimestep = 
            std::chrono::duration<double>(1.0f / 600.0f);
        // time elapsed since last frame render
        std::chrono::duration<double> m_frameTimeRemaining = 
            std::chrono::duration<double>(0.0);
    };
}

#endif // I_COSMOS_CONTEXT_H