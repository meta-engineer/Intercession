#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

// pass entity values to register
#include "ecs/ecs_types.h"
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
        }

        // events pertaining to the rendering system
        namespace rendering {
            const EventId SET_MAIN_CAMERA = __LINE__;
                struct SET_MAIN_CAMERA_params
                {
                    Entity cameraEntity = NULL_ENTITY;
                };
        }

        // cross-concerning events that exist within the virtual verse
        // maybe the cosmos should have it's own seperate broker which can be erased when context changes cosmos
        namespace cosmos {
            // notify that an entity SIGNATURE has been modified locally
            // and those changes should be propagated to other servers/clients
            // Does not include the content/members of those components
            const EventId ENTITY_MODIFIED = __LINE__;
                struct ENTITY_MODIFIED_params
                {
                    Entity entity = NULL_ENTITY;
                    Signature sign;
                };
            
            // Indicates that the cosmos has added a new entity
            // (ambiguous whether WE created it, or we registered it from another host)
            // Entity will exist when this is signalled
            // (sent over a network implies that the remote has added this new entity)
            const EventId ENTITY_CREATED = __LINE__;
                struct ENTITY_CREATED_params
                {
                    Entity entity = NULL_ENTITY;
                    // initialize components after creation
                    Signature sign;
                };
            
            // upon deleting an entity we may need to notify its host
            // CAREFUL! entity will NOT exist when this is signalled
            const EventId ENTITY_REMOVED = __LINE__;
                struct ENTITY_REMOVED_params
                {
                    Entity entity = NULL_ENTITY;
                };
            // Request for an entity to be removed at the end of the frame
            // once all unsafe references are cleared
            const EventId CONDEMN_ENTITY = __LINE__;
                struct CONDEMN_ENTITY_params
                {
                    Entity entity = NULL_ENTITY;
                };
            // Request for all entities to be removed from cosmos
            const EventId CONDEMN_ALL = __LINE__;
            
            // entity update will be a dynamically packed series of components:
            // Each consecutive component represented in the entity's signature.
            // (assuming the APP_INFO checks out the component layout should match)
            // then the sign (Signature: 32 bits),
            // then the entity (Entity: 16 bits)
            // (remember it is a FIFO stack)
            const EventId ENTITY_UPDATE = __LINE__;
                struct ENTITY_UPDATE_params {
                    Entity entity = NULL_ENTITY;
                    Signature sign;
                };

            // Request for the whole cosmos to be remade
            // (removing all entities, components, and syncrhos)
            // not be guarenteed to happen immediately, but before next frame
            //const EventId RESET = __LINE__;
        }

        // events used as network messages sent over net::Connection
        // may be forwarded to be sent over local EventBroker
        namespace network {
            // Generic/basic app properties
            // Should probably be only info generic to all potential apps for consistent size
            // different versionMinor should indicate apps are incompatible
            const EventId PROGRAM_INFO = __LINE__;
                struct PROGRAM_INFO_params
                {
                    char name[32]        = BUILD_PROJECT_NAME;
                    uint8_t versionMajor = BUILD_VERSION_MAJOR;
                    uint8_t versionMinor = BUILD_VERSION_MINOR;
                    uint8_t versionPatch = BUILD_VERSION_PATCH;
                    
                    bool is_compatible(const PROGRAM_INFO_params& other)
                    {
                        return (strcmp(this->name, other.name) == 0
                            && this->versionMajor == other.versionMajor
                            && this->versionMinor == other.versionMinor);
                    }
                    
                    friend bool operator==(const PROGRAM_INFO_params& lhs, const PROGRAM_INFO_params& rhs)
                    {
                        return (strcmp(lhs.name, rhs.name) == 0
                            && lhs.versionMajor == rhs.versionMajor
                            && lhs.versionMinor == rhs.versionMinor
                            && lhs.versionPatch == rhs.versionPatch);
                    }
                };

            // Info about the app configuration specifically:
            //  Am I a server or a client (or a dispatch)
            //  What is the ID of my cluster/server group?
            //  How many servers are in my cluster?
            //  what is my client id/server id
            //  What is the address of the other servers (is this required?)
            //  What is the synchronized timepoint (minus my local delay)
            const EventId APP_INFO = __LINE__;
                struct APP_INFO_params
                {
                    // pass CosmosBuilder::Config?
                    // pass timeline_config?
                };

            // Info about a timestream modification
            //  ???
            const EventId SUPERPOSITION = __LINE__;
                struct SUPERPOSITION_params
                {
                    // time-travelling entity
                    Entity agent;
                    // Entity who was changed by a time-traveller
                    Entity recipient;
                };
                
            // Servers receive this as a request froma  client to be added to the cosmos
            // Clients receive this as an acknowledgment that their request was received
            const EventId NEW_CLIENT = __LINE__;
                struct NEW_CLIENT_params
                {
                    // shared pointer to remote already contained in OwnedMessage

                    // Used to notify server that client is transferring from another timeslice
                    // 0 indicates no transfer (brand-new)
                    // value should be copied from a received JUMP_RESPONSE
                    uint32_t transferCode = 0;

                    // used by server to respond to new client with their assigned focal entity
                    Entity entity = 0;
                };

            // Server reports the current coherency at time of sending
            const EventId COHERENCY_SYNC = __LINE__;
                struct COHERENCY_SYNC_params
                {
                    TimesliceId senderId;
                    // use coherency from message header
                    //uint16_t coherency;
                };

            // Request timetravel
            // entity will be serialized into the Message before _params
            const EventId JUMP_REQUEST = __LINE__;
                struct JUMP_REQUEST_params
                {
                    // relative number of timeslices requested to jump (negative is to future)
                    // Should this be relative or absolute?
                    int timesliceDelta = 0;

                    // assigned by source timeslice then reflected back by destination in JUMP_RESPONSE
                    uint32_t requesterConnId;

                    // Entity belonging to address who is jumping
                    // REMEMBER to serialize this entity in message before adding params!
                    Entity entity = NULL_ENTITY;
                    Signature sign;
                };
            // Reponse returned by destination timeslice to confirm that they are ready to receive the time traveller
            const EventId JUMP_RESPONSE = __LINE__;
                struct JUMP_RESPONSE_params
                {
                    // return client connection id which server provided us in the JUMP_REQUEST
                    uint32_t requesterConnId = 0;

                    // generated secret code for client to use when they connect
                    // 0 implies failure, do not jump
                    uint32_t transferCode = 0;

                    // inform client of where to connect
                    // they must already know the address
                    uint16_t port = 0;
                };
        }
    }
}

#endif // EVENT_TYPES_H