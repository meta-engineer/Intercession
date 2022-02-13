#ifndef EVENT_H
#define EVENT_H

//#include "intercession_pch.h"
#include <unordered_map>
#include <array>
#include <exception>

#include "event_types.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class Event
    {
    public:
        Event() = delete;
        explicit Event(EventId type);

        EventId get_type() const;

        template<typename T>
        void set_param(T paramStruct);

        template<typename T>
        T get_param();


    private:
        EventId m_type;

        // how to store unknown type of variable size data? std::any ?
        // store a static sized byte array as large as the largest param struct
        // memcpy struct into byte array.
        // Event receiver will copy it back out
        std::array<uint8_t, MAX_PARAM_SIZE> paramBytes;
    };

  
    inline Event::Event(EventId type)
        : m_type(type)
    {}

    inline EventId Event::get_type() const
    {
        return m_type;
    }

    template<typename T>
    void Event::set_param(T paramStruct) 
    {
        // sizeof() is in bytes
        const bool paramTooLarge = sizeof(T) > (sizeof(uint8_t) * MAX_PARAM_SIZE);
        if (paramTooLarge)
        {
            PLEEPLOG_ERROR("Trying to set parameter of size " + std::to_string(sizeof(T)) + " greater than max event size " + std::to_string(sizeof(uint8_t) * MAX_PARAM_SIZE));
            throw std::range_error("Event set_param request was too large");
        }

        std::memcpy(paramBytes.data(), &paramStruct, sizeof(T));
    }
    
    template<typename T>
    T Event::get_param() 
    {
        // sizeof() is in bytes
        const bool paramTooLarge = sizeof(T) > (sizeof(uint8_t) * MAX_PARAM_SIZE);
        if (paramTooLarge)
        {
            PLEEPLOG_ERROR("Trying to get parameter of size " + std::to_string(sizeof(T)) + " greater than max event size " + std::to_string(sizeof(uint8_t) * MAX_PARAM_SIZE));
            throw std::range_error("Event get_param request was too large");
        }

        T paramStruct;
        std::memcpy(&paramStruct, paramBytes.data(), sizeof(T));
        return paramStruct;
    }
}

#endif // EVENT_H