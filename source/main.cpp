// Copywright Pleep inc. 2022

// TODO: precompiled header

// std libraries
#include <iostream>
#include <string>
#include <vector>

// internal
#include "build_config.h"
#include "logging/pleep_log.h"
#include "core/app_gateway.h"

int main(int argc, char** argv)
{
    INIT_PLEEPLOG();
    PLEEPLOG_TRACE("Initialized pleep logger!");

    PLEEPLOG_INFO(argv[0]);
    PLEEPLOG_INFO("Build version: " + std::to_string(BUILD_VERSION_MAJOR) + "." + std::to_string(BUILD_VERSION_MINOR));

#if defined(_DEBUG)
    PLEEPLOG_INFO("Build config:  Debug");
#elif defined(NDEBUG)
    PLEEPLOG_INFO("Build config:  Release");
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


    pleep::AppGateway* client = nullptr;

    // top level, last-resort catch to safely handle errors
    try
    {
        // pass config resources to build context and initial state
        client = new pleep::AppGateway();
    }
    catch (const std::exception& e)
    {
        PLEEPLOG_ERROR("The following uncaught exception occurred during AppGateway startup: ");
        PLEEPLOG_ERROR(e.what());
        return 1;
    }

    try
    {
        // "run" is synchronous, returning implies app has stopped (unlike "start")
        client->run();
    }
    catch (const std::exception& e)
    {
        UNREFERENCED_PARAMETER(e);
        PLEEPLOG_ERROR("Uncaught exception during AppGateway running");
        PLEEPLOG_ERROR(e.what());
        return 1;
    }

    // cleanup
    delete client;

    return 0;
}
