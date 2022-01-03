// Copywright Pleep inc. 2022

// TODO: precompiled header

// std libraries
#include <iostream>
#include <string>
#include <vector>

// internal
#include "build_config.h"
#include "core/app_gateway.h"

int main(int argc, char** argv)
{
    // TODO: PLEEP_LOG()
    std::cout << argv[0] << std::endl;
    std::cout << "Build version: " << BUILD_VERSION_MAJOR << "." << BUILD_VERSION_MINOR << std::endl;

#if defined(_DEBUG)
    std::cout << "Build config:  Debug" << std::endl;
#elif defined(NDEBUG)
    std::cout << "Build config:  Release" << std::endl;
#endif

    // parse cmd arguments
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);

    if (!args.empty())
    {
        // just echo for now
        std::cout << "Ignored args: ";
        for (std::string a : args)
            std::cout << a << " | ";
        std::cout << std::endl << std::endl;
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
        // TODO: PLEEPLOG_ERROR() with errorcodes for clients in release
        //  also PLEEPLOG_DEBUG() with file and linenumber for devs in debug
        std::cerr << "main:: Uncaught exception during AppGateway startup" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    try
    {
        client->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "main:: Uncaught exception during AppGateway running" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    // cleanup
    delete client;

    return 0;
}
