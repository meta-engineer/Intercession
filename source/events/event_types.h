#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

// pass entity values to register
#include "ecs/ecs_types.h"

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

    using EventId = std::uint32_t;
    // size is in bytes
    // int is 4 bytes
    constexpr size_t MAX_PARAM_SIZE = 8;

    // namespaces can nest event classifications in a tree
    // but their values must be generated uniquely otherwise
    namespace events {
        // outside events from window api
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

        // cross-concerning events that exist within the virtual verse
        namespace cosmos {

        }

        // events pertaining to the rendering system
        namespace rendering {
            const EventId SET_MAIN_CAMERA = 3;
            namespace set_main_camera {
                struct Params {
                    Entity cameraEntity;
                };
            }
        }
    }
}

#endif // EVENT_TYPES_H