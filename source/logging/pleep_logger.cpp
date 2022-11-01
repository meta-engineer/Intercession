#include "pleep_logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "logging/pleep_log.h"

namespace pleep
{
#ifdef PLEEPLOG_ON
    // define static logger
    std::shared_ptr<spdlog::logger> PleepLogger::s_coreLogger;

    PleepLogger::PleepLogger() 
    {
        
    }
    
    PleepLogger::~PleepLogger() 
    {
        
    }
    
    void PleepLogger::init() 
    {
        // Cherno
        // "%^[%T] %n: %v%$"
        // Hdlm
        // "%^[%D][%T][%s:%#][%^%l%$] %v"
        // %t for thread id
        // %P for process id

        s_coreLogger = spdlog::stdout_color_mt("INTERCESSION");
        s_coreLogger->set_pattern("[%T.%e] #%-5t %28!s:%-4#|%^%6!l%$: %v");
        s_coreLogger->set_level(PLEEPLOG_LEVEL);

        PLEEPLOG_TRACE("Initialized pleep logger!");
    }
    
    std::shared_ptr<spdlog::logger>& PleepLogger::GetPleepLogger() 
    {
        return s_coreLogger;
    }
#endif
}