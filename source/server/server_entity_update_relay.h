#ifndef SERVER_ENTITY_UPDATE_RELAY_H
#define SERVER_ENTITY_UPDATE_RELAY_H

//#include "intercession_pch.h"
#include "networking/a_network_relay.h"
#include "events/event_types.h"
#include "networking/timeline_types.h"

namespace pleep
{
    class ServerEntityUpdateRelay : public A_NetworkRelay
    {
    public:
        ServerEntityUpdateRelay(EventBroker* contextBroker = nullptr)
            // Cannot "initialize" m_sharedBroker because it already done after A_NetworkRelay constructor
        {
            m_sharedBroker = contextBroker;
            // Register for events emitted by server interface
            m_sharedBroker->add_listener(METHOD_LISTENER(events::network::ENTITY_UPDATE, ServerEntityUpdateRelay::_entity_update_handler));
        }
        ~ServerEntityUpdateRelay() = default;

        void engage(double deltaTime) override
        {
            UNREFERENCED_PARAMETER(deltaTime);
            // Should relays track these event data, or respond to them immediately when
            // they are called to by network interface
            // network interface calls are not async (called on dynamo run_relays)
            // so they will not pre-empt other per-frame sections

            // if updates are processed immediately (on event callback) then there is nothing to do here
            // we rely on the event emitter to be responsible for proper pipeline ordering
            
            // If relays DO NOT react immediately then we cannot synchronize
            // message ordering between multiple relays.
            // After processing Message A, when we process Message B, we might expect Message A
            // to also have been processed by another relay in parallel
            
            // Do we care about processing ordered-by-relay
            // OR do we prefer processing ordered-by-message?
        }
        

    private:
        // inherits A_NetworkRelay::m_workingCosmos
        // inherits A_NetworkRelay::m_sharedBroker

        void _entity_update_handler(EventMessage updateEvent)
        {
            events::network::ENTITY_UPDATE_params updateData;
            updateEvent >> updateData;
            PLEEPLOG_DEBUG("Handling entity update event for TemporalEntity: " + std::to_string(updateData.id) + ", link: " + std::to_string(updateData.link));

            // convert timeline to local entity id
            // m_workingCosmos->get_local_id(updateData.id, updateData.link);
            Entity localId = NULL_ENTITY; // TODO
            PLEEPLOG_DEBUG("Processing entity update for Entity: " + std::to_string(localId) + " (" + updateData.sign.to_string() + ")");
            // confirm if (data sign == localID sign)

            if (localId == NULL_ENTITY)
            {
                // entity with given timeline id doesn't exist?
                PLEEPLOG_WARN("Ignoring update for null local entity");
                return;
            }
            
            // TODO: also need to consider latency. If we know the player actions ocurred ~50ms ago
            // which is ~5 physics updates, how do we reason about the current state?

            // immediate response mode:
            for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
            {
                if (updateData.sign.test(i))
                {
                    std::string componentName = m_workingCosmos->get_component_name(i);

                    // TODO: switch on known component names:
                    // stream>> them myself, interrogate their values, and then overwrite the ecs

                    // default:
                    //m_workingCosmos->deserialize_and_write_component(updateEvent, componentName, updateData.id);
                }
            }
        }
    };
}

#endif // SERVER_ENTITY_UPDATE_RELAY_H