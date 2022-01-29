#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

namespace pleep
{
    // we cant use this because it doesn't implicitly convert
    //using EventHandler = std::function<void(Event&)>;

    // austin morlan's macro for generating function pointer wrappers
    #define METHOD_LISTENER(EventType, Listener) EventType, std::bind(&Listener, this, std::placeholders::_1)
    #define FUNCTION_LISTENER(EventType, Listener) EventType, std::bind(&Listener, std::placeholders::_1)


    // Event Type Definitions
    // enums conveniently assign values to each event
    // but all events would have to be under the same enum
    //   otherwise when they cast to EventId they will create collisions
    // we can define event types without an enum using a unique hash

    // how to attach some data to an event.
    // EX: window resize event needs to contain the dimensions of the new size

    using EventId = std::uint32_t;
    //using ParamId = std::uint32_t;

    // namespaces can nest event classifications in a tree
    // but their values must be generated uniquely otherwise
    namespace events {
        namespace window {
            const EventId QUIT = 0;
            const EventId RESIZE = 1;
            namespace resize {
                struct Params {
                    int width;
                    int height;
                };
            }
            const EventId INPUT = 2;
            namespace input {
                // bitset of keys values? single key values? command abstraction (for remapping/controllers)
                struct Params {
                    
                };
            }
        }
    }
}

#endif // EVENT_TYPES_H