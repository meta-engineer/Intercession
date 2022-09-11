#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

// pass entity values to register
#include "ecs/ecs_types.h"
#include "networking/timeline_types.h"
#include "events/message.h"
#include "build_config.h"

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
    using EventMessage = Message<EventId>;

    // namespaces can nest event classifications in a tree
    // but their values must be generated uniquely otherwise
    namespace events {
        // outside events from window api
        namespace window {
            const EventId QUIT = __LINE__;
            const EventId RESIZE = __LINE__;
                struct RESIZE_params
                {
                    int width = 0;
                    int height = 0;
                };
            const EventId INPUT = __LINE__;
                struct INPUT_params
                {
                    // bitset of keys values? single key values? command abstraction (for remapping/controllers)
                };
        }

        // cross-concerning events that exist within the virtual verse
        // maybe the cosmos should have it's own seperate broker which can be erased when context changes cosmos
        namespace cosmos {
            // notify that an entity has been modified locally (not specific to any component)
            // and those changes should be propagated to other servers/clients
            const EventId ENTITY_MODIFIED = __LINE__;
                struct ENTITY_MODIFIED_params
                {
                    Entity entityId = NULL_ENTITY;
                };
        }

        // events pertaining to the rendering system
        namespace rendering {
            const EventId SET_MAIN_CAMERA = __LINE__;
                struct SET_MAIN_CAMERA_params
                {
                    Entity cameraEntity = NULL_ENTITY;
                };
        }

        // events used as network messages sent over net::Connection
        // may be forwarded to be sent over local EventBroker
        namespace network {
            // Generic/basic app properties
            // Should probably be only info generic to all potential apps for consisstent size
            const EventId APP_INFO = __LINE__;
                struct APP_INFO_params
                {
                    char name[32]        = BUILD_PROJECT_NAME;
                    uint8_t versionMajor = BUILD_VERSION_MAJOR;
                    uint8_t versionMinor = BUILD_VERSION_MINOR;
                    uint8_t versionPatch = BUILD_VERSION_PATCH;
                    
                    friend bool operator==(const APP_INFO_params& lhs, const APP_INFO_params& rhs)
                    {
                        return (strcmp(lhs.name, rhs.name) == 0
                            && lhs.versionMajor == rhs.versionMajor
                            && lhs.versionMinor == rhs.versionMinor
                            && lhs.versionPatch == rhs.versionPatch);
                    }
                };
            // Info about the Intercession app configuration specifically:
            //  Am I a server of a client (or a dispatch)
            //  What is the ID of my cluster/server group?
            //  How many servers are in my cluster?
            //  what is my client id/server id
            //  What is the address of the other servers (is this required?)
            //  What is the synchronized timepoint (minus my local delay)
            const EventId INTERCESSION_APP_INFO = __LINE__;
                struct INTERCESSION_APP_INFO_params
                {

                };
            // Info about the current cosmos:
            //  Running state
            //  total registered synchros, components, and entities
            //  checksum of registered synchros & components
            const EventId COSMOS_INFO = __LINE__;
                struct COSMOS_INFO_params
                {

                };
            // entity update will be a dynamically packed series of components:
            // Each consecutive component represented in the entity's signature.
            // assuming the intercessionAppInfo checks out the component layout should match
            // then the id (TemporalEntity: 16 bits),
            // the link (CausalChainLink: 8 bits)
            // the sign (Signature: 32 bits)
            // (remember it is a FIFO stack)
            const EventId ENTITY_UPDATE = __LINE__;
                struct ENTITY_UPDATE_params {
                    TemporalEntity id = NULL_ENTITY;
                    CausalChainLink link;
                    Signature sign;
                };
            // Notify the creation of an entity
            // this should trigger TemporalEntity id count update (as well as creating the ecs objects)
            // a subsequent ENTITY_UPDATE can pass the data for the created components
            // (This may have to be independantly propagated in the timestream because the ServerEntityUpdateRelay only detects existing entities)
            // Careful of race-conditions where entity is without data.
            // ENTITY_CREATE must be passed into the timestream FIRST before any updates
            const EventId ENTITY_CREATE = __LINE__;
                struct ENTITY_CREATE_params {
                    TemporalEntity id = NULL_ENTITY;
                    CausalChainLink link;
                    Signature sign;
                };
            // Inverse of ENTITY_CREATE
            const EventId ENTITY_DELETE = __LINE__;
                struct ENTITY_DELETE_params {
                    TemporalEntity id = NULL_ENTITY;
                    CausalChainLink link;
                    Signature sign;
                };
            // Info about a timestream modification
            //  ???
            const EventId INTERCESSION_UPDATE = __LINE__;
                struct INTERCESSION_UPDATE_params
                {

                };
        }
    }
}

#endif // EVENT_TYPES_H