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
    PLEEPLOG_INFO("Intercession server app");
    PLEEPLOG_INFO("Build version: " + std::to_string(BUILD_VERSION_MAJOR) + "." + std::to_string(BUILD_VERSION_MINOR));

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

    // Construct server specific AppGateway
    pleep::AppGateway* intercessionServer = nullptr;

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
