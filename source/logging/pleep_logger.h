#ifndef PLEEP_LOGGER_H
#define PLEEP_LOGGER_H

//#include "intercession_pch.h"
#include <memory>

// also defined in glfw, glfw has redefine guard, spdlog does not
#undef APIENTRY
#include "spdlog/spdlog.h"

// ***** Logger compilation options *****

#if defined(_DEBUG)
// 1. toggle log objects and macros
#define PLEEPLOG_ON
#endif

// 2. change minimum displayed log level
#define PLEEPLOG_LEVEL spdlog::level::trace

// TODO: sink logging to file?
//#define PLEEPLOG_FILE
//#define PLEEPLOG_CONSOLE

// TODO: have error/critical create popup for non-technical users to report remotely

// *********** End of options ***********

namespace pleep
{
#ifdef PLEEPLOG_ON
    class PleepLogger
    {
    public:
        PleepLogger();
        ~PleepLogger();

        static void init();

        static std::shared_ptr<spdlog::logger>& GetPleepLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_coreLogger;
    };
#endif
}

#endif // PLEEP_LOGGER_H