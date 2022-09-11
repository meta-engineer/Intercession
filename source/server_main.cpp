// Copyright Pleep inc. 2022

// TODO: precompiled header

// std libraries
#include <string>
#include <vector>

// internal
#include "build_config.h"
#include "logging/pleep_log.h"
#include "server/server_app_gateway.h"

int main(int argc, char** argv)
{
    INIT_PLEEPLOG();

    PLEEPLOG_INFO(argv[0]);
    PLEEPLOG_INFO(BUILD_PROJECT_NAME " server app");
    PLEEPLOG_INFO("Build version: " + std::to_string(BUILD_VERSION_MAJOR) + "." + std::to_string(BUILD_VERSION_MINOR) + "." + std::to_string(BUILD_VERSION_PATCH));

#if defined(_DEBUG)
    PLEEPLOG_INFO("Build config:  Debug");
#elif defined(NDEBUG)
    PLEEPLOG_INFO("Build config:  Release");
#else
    PLEEPLOG_INFO("Build config:  Undefined");
#endif

    // parse cmd arguments
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);

    if (!args.empty())
    {
        // just echo for now
        std::string concat;
        for (std::string a : args)
            concat.append(a + " ");

        PLEEPLOG_WARN("Ignored cmd args: " + concat);
    }

    // TODO: Read/use some config (accept json file path on cmd?) 
    //   and setup multiple threads running seperate CosmosContexts
    // config should describe the what starting cosmos' to build, network topology, addresses to use, network update Hz (tick rate)
    // (and potentially other non-network configs like physics update Hz)
    // AppGateway will setup inter-cosmos-communication over a local-temporal-network
    // and expose it to the Context for each timeslice

    // TODO: Using pleeplog for multiple threads might be unwieldly
    // We can make a simple Terminal UI to display state for each servers in
    // the cluster even in release mode (and avoid async threads debug logs
    // crowding eachother. Assuming because they are all the same process we
    // can't spawn multiple windows)
    // Like the carriage return trick we can use escape sequences to rewrite the
    // screen in realtime, and read keystrokes with std::getchar() to
    // make it interactive? (as simple as <cstdio>?)
    // ANSI Escape Sequences:
    // https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797

    // Whatever terminal "display" I build it will have to safely mesh
    // with or disable PLEEPLOG, either making them exclusive build options
    // or overriding the PLEEPLOG macros to display them in the UI instead.
    // (get spdlog to feed its generated messages into a circular buffer)

    // Construct server specific AppGateway
    pleep::I_AppGateway* intercessionServer = nullptr;

    // top level, last-resort catch to safely handle errors
    try
    {
        // pass config resources to build context and initial state
        intercessionServer = new pleep::ServerAppGateway();
    }
    catch (const std::exception& e)
    {
        UNREFERENCED_PARAMETER(e);
        PLEEPLOG_ERROR("The following uncaught exception occurred during ServerAppGateway startup: ");
        PLEEPLOG_ERROR(e.what());
        return 1;
    }

    try
    {
        // "run" is synchronous, returning implies app has stopped (unlike "start")
        intercessionServer->run();
    }
    catch (const std::exception& e)
    {
        UNREFERENCED_PARAMETER(e);
        PLEEPLOG_ERROR("Uncaught exception during ServerAppGateway running");
        PLEEPLOG_ERROR(e.what());
        return 1;
    }

    // cleanup
    delete intercessionServer;

    return 0;
}
