#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

// pass entity values to register
#include "ecs/ecs_types.h"
#include "networking/net_message.h"

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
    // borrow network message serialization to package event messages
    using EventMessage = Message<EventId>;

    // namespaces can nest event classifications in a tree
    // but their values must be generated uniquely otherwise
    namespace events {
        // outside events from window api
        namespace window {
            const EventId QUIT = __LINE__;
            const EventId RESIZE = __LINE__;
                struct RESIZE_params {
                    int width = 0;
                    int height = 0;
                };
            const EventId INPUT = __LINE__;
                struct INPUT_params {
                    // bitset of keys values? single key values? command abstraction (for remapping/controllers)
                };
        }

        // cross-concerning events that exist within the virtual verse
        // maybe the cosmos should have it's own seperate broker which can be erased when context changes cosmos
        namespace cosmos {
            const EventId ENTITY_MODIFIED = __LINE__;
                struct ENTITY_MODIFIED_params {
                    Entity entityId = NULL_ENTITY;
                };
        }

        // signal to the net dynamo to proc communication/updates
        namespace network {

        }

        // events pertaining to the rendering system
        namespace rendering {
            const EventId SET_MAIN_CAMERA = __LINE__;
                struct SET_MAIN_CAMERA_params {
                    Entity cameraEntity = NULL_ENTITY;
                };
        }
    }
}

#endif // EVENT_TYPES_H