#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

namespace pleep
{
    using EventId = std::uint32_t;
    //using ParamId = std::uint32_t;

    class Event;
    // we cant use this because it doesn't implicitly convert
    //using EventHandler = std::function<void(Event&)>;

    // austin morlan's macro for generating function pointer wrappers
    #define METHOD_LISTENER(EventType, Listener) EventType, std::bind(&Listener, this, std::placeholders::_1)
    #define FUNCTION_LISTENER(EventType, Listener) EventType, std::bind(&Listener, std::placeholders::_1)

    // Define event consts
    // enums conveniently assign values to each event
    // but all events would have to be under the same enum
    // otherwise when they cast to EventId they will create collisions

    // namespaces can nest event classifications in a tree
    // but their values must be generated uniquely otherwise
    namespace events {
        namespace window {
            const EventId QUIT = 0;
            const EventId RESIZE = 1;
            namespace resize {
                //const ParamId WIDTH = ...
                //const ParamId HEIGHT = ...
            }
            const EventId INPUT = 2;
            namespace input {
                // how to comunicate inputs?
                // bitset of keys values? single key value? command abstraction (for remapping/controllers)
                //const ParamId CODES = ...
            }
        }
    }
}

#endif // EVENT_TYPES_H