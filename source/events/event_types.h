#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

//#include "intercession_pch.h"
#include <cstdint>
#include <functional>

// pass entity values to register
#include "ecs/ecs_types.h"
#include "events/message.h"
#include "build_config.h"
#include "spacetime/timestream_state.h"

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
                // no QUIT_params needed
            const EventId RESIZE = __LINE__;
                struct RESIZE_params
                {
                    int width = 0;
                    int height = 0;
                };
            // "virtual" input device which can use 3D coordinates
            const EventId VIRTUAL_ODM_GEAR_INPUT = __LINE__;
                struct VIRTUAL_ODM_GEAR_INPUT_params
                {
                    double x = 0.0;
                    double y = 0.0;
                    double z = 0.0;
                };
        } // namespace window

        // events pertaining to the rendering system
        namespace rendering {
            const EventId SET_MAIN_CAMERA = __LINE__;
                struct SET_MAIN_CAMERA_params
                {
                    Entity cameraEntity = NULL_ENTITY;
                };
        } // namespace rendering

        // cross-concerning events that exist within the virtual verse
        // maybe the cosmos should have it's own seperate broker which can be erased when context changes cosmos
        namespace cosmos {
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

                    // entity which caused the creation of this one (cannot be the same as entity)
                    // null implies this entity was created by something outside of the cosmos (forces creation to be valid)
                    Entity source = NULL_ENTITY;
                };
            
            // upon deleting an entity we may need to notify its host
            // CAREFUL! entity will NOT exist when this is signalled
            const EventId ENTITY_REMOVED = __LINE__;
                struct ENTITY_REMOVED_params
                {
                    Entity entity = NULL_ENTITY;

                    // entity which cause the deletion of the one (CAN be the same as entity)
                    // null implies this entity was deleted by something outside of the cosmos (forces deletion to be valid)
                    Entity source = NULL_ENTITY;
                };
            // Request for all entities to be removed from cosmos
            const EventId CONDEMN_ALL = __LINE__;
            
            // entity update will be a dynamically packed series of components:
            // Each consecutive component represented in the entity's signature.
            // (assuming the APP_INFO checks out the component layout should match)
            // THEN the following params struct (remember it is a FIFO stack)
            const EventId ENTITY_UPDATE = __LINE__;
                struct ENTITY_UPDATE_params {
                    Entity entity = NULL_ENTITY;
                    // indicates which components are serialized in this message
                    // and therefore the components you must deserialize (even if you discard some)
                    Signature sign;
                    // indicates which components from the sign to use
                    // (only ComponentCategory::all implies removal of components)
                    ComponentCategory category;
                };

            // Request for the whole cosmos to be remade
            // (removing all entities, components, and syncrhos)
            // not be guarenteed to happen immediately, but before next frame
            //const EventId RESET = __LINE__;

            // Signals an entity has changed its timestream state
            // This is not a command, but simply a notification of occurrance 
            // for the receiver to interpret however they wish
            const EventId TIMESTREAM_STATE_CHANGE = __LINE__;
                struct TIMESTREAM_STATE_CHANGE_params {
                    // Entity who has _already_ been changed
                    Entity entity;
                    // State that entity transitioned to
                    TimestreamState newState;
                    // location change occurred
                    TimesliceId timeslice;
                };

            // Signals an entity which was "merged" with the timestream has been "forked"
            //   (by a time-traveller)
            const EventId TIMESTREAM_INTERCEPTION = __LINE__;
                struct TIMESTREAM_INTERCEPTION_params {
                    // time-travelling entity
                    Entity agent;
                    // Entity who was changed by a time-traveller
                    Entity recipient;
                };

            // Signals an object has caused a paradox?
            // manual rewriting of timestream for entity?
            // and/or new entities generated due to the intercession?
            const EventId TIMESTREAM_INTERCESSION = __LINE__;
                struct TIMESTREAM_INTERCESSION_params {
                    ///...
                };
        } // namespace cosmos

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

                    TimesliceId currentTimeslice = 0;
                    size_t totalTimeslices = 0;
                };
                
            // Servers receive this as a request froma  client to be added to the cosmos
            // Clients receive this as an acknowledgment that their request was received
            const EventId NEW_CLIENT = __LINE__;
                struct NEW_CLIENT_params
                {
                    // shared pointer to remote already contained in OwnedMessage

                    // Used to notify server that client is transferring from another timeslice
                    // 0 indicates no transfer (brand-new)
                    // value should be copied from a received JUMP_ARRIVAL
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

            // Indicates an entity is intending to timetravel
            // contains serialized entity
            const EventId JUMP_REQUEST = __LINE__;
            // indicates entity is about to leave a timeslice
            // No serialized data (not necessary)
            const EventId JUMP_DEPARTURE = __LINE__;
            // indicates entity has just arrived in a timeslice
            // contains serialized entity
            const EventId JUMP_ARRIVAL = __LINE__;
                // All JUMP_ events can just use the same _params struct:
                struct JUMP_params
                {
                    // relative number of timeslices requested to jump (negative is to future)
                    // relative, so that it works natively after being propogated
                    // departures and arrivals should be inverse
                    int timesliceDelta = 0;

                    // Entity who is jumping
                    // REMEMBER to serialize this entity in message before adding params!
                    Entity entity = NULL_ENTITY;
                    Signature sign;

                    // unique identifier for matching departure&arrival pairs
                    // (usually generated from timestamp)
                    // 0 implies failure, do not jump
                    uint32_t tripId;

                    // indicates that this is the focal entity for a client
                    bool isFocal = false;
                    
                    // inform client of where to connect
                    // they must already know the address?
                    uint16_t port = 0;
                };
        } // namespace network

        // events used for managing/communicating parallel cosmos state
        namespace parallel
        {
            // parallel wants to start cycle from recipients timeslice
            // and is requesting initialization
            const EventId INIT = __LINE__;
                struct INIT_params
                {
                    TimesliceId sourceTimeslice;
                };
            // simulation has finished and reached target coherency point
            const EventId FINISHED = __LINE__;
                struct FINISHED_params
                {
                    TimesliceId destinationTimeslice;
                    // use coherency from message header
                };
            // indicates a divergence has occurred the given timeslice
            // if not running, parallel should send an init to that slice
            // if yes running, parallel should cycle when it reaches the present
            const EventId DIVERGENCE = __LINE__;
                struct DIVERGENCE_params
                {
                    TimesliceId sourceTimeslice;
                };
            // indicates a paradox is being resolved and this entity should merged in from the currently existing worldline
            // (it should also increment worldlines) 
            const EventId WORLDLINE_SHIFT = __LINE__;
                struct WORLDLINE_SHIFT_params
                {
                    // entity should(?) always be a chainlink 0 entity
                    Entity entity;
                    // sign of any packed data
                    Signature sign;
                };
        } // namespace parallel
    }
}

#endif // EVENT_TYPES_H