#ifndef PLEEP_LOG_H
#define PLEEP_LOG_H

#include "pleep_logger.h"

// ^^^ See pleep_logger.h for configuration options

// predefined macro to supply meta data
//SPDLOG_LOGGER_DEBUG(GET_PLEEP_LOGGER(), __VA_ARGS__))

#ifdef PLEEPLOG_ON
    #define INIT_PLEEPLOG() pleep::PleepLogger::init()
    #define GET_PLEEP_LOGGER() pleep::PleepLogger::GetPleepLogger()

    #define PLEEPLOG_TRACE(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::trace, __VA_ARGS__)

    #define PLEEPLOG_DEBUG(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::debug, __VA_ARGS__)

    #define PLEEPLOG_INFO(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::info, __VA_ARGS__)

    #define PLEEPLOG_WARN(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::warn, __VA_ARGS__)

    #define PLEEPLOG_ERROR(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::err, __VA_ARGS__)

    #define PLEEPLOG_CRITICAL(...) GET_PLEEP_LOGGER()->log(spdlog::source_loc{__FILE__, __LINE__, static_cast<const char *>(__FUNCTION__)}, spdlog::level::critical, __VA_ARGS__)
#else
    #define INIT_PLEEPLOG()
    #define GET_PLEEP_LOGGER()

    #define PLEEPLOG_TRACE(...)

    #define PLEEPLOG_DEBUG(...)

    #define PLEEPLOG_INFO(...)

    #define PLEEPLOG_WARN(...)

    #define PLEEPLOG_ERROR(...)

    #define PLEEPLOG_CRITICAL(...)
#endif

#endif // PLEEP_LOG_H