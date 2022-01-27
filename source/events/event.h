#ifndef EVENT_H
#define EVENT_H

//#include "intercession_pch.h"
#include <unordered_map>

#include "event_types.h"

namespace pleep
{
    class Event
    {
    public:
        Event() = delete;
        explicit Event(EventId type);

        //template<typename T>
        //void set_param(EventId id, T value);

        //template<typename T>
        //T get_param(EventId id);

        EventId get_type() const;

    private:
        EventId m_type;
        // how to store unknown type of variable size data?
        // std::any ?
    };
}

#endif // EVENT_H