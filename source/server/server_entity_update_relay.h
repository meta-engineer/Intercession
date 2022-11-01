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
        ServerEntityUpdateRelay()
        {
        }
        ~ServerEntityUpdateRelay() = default;

        void engage(double deltaTime) override
        {
            UNREFERENCED_PARAMETER(deltaTime);
            
            for (std::vector<EventMessage>::iterator message_it = m_networkMessages.begin(); message_it != m_networkMessages.end(); message_it++)
            {
                EventMessage& updateMsg = *message_it;

                events::network::ENTITY_UPDATE_params updateData;
                updateMsg >> updateData;
                PLEEPLOG_DEBUG("Handling entity update event for TemporalEntity: " + std::to_string(updateData.id) + ", link: " + std::to_string(updateData.link));
                
                // convert timeline to local entity id
                Entity localId = m_workingCosmos->get_local_entity(updateData.id, updateData.link);
                if (localId == NULL_ENTITY)
                {
                    PLEEPLOG_WARN("Tried to update TemporalId which does not exist locally, it must be created first, skipping...");
                    return;
                }

                PLEEPLOG_DEBUG("Processing entity update for Entity: " + std::to_string(localId) + " (" + updateData.sign.to_string() + ")");
                // confirm if (data sign == localID sign)
                if (updateData.sign != m_workingCosmos->get_entity_signature(localId))
                {
                    PLEEPLOG_WARN("Updated entity data has mismatching local signature. Should we implicity create one, or should we expect an explicit \"add component\" event? Skipping for now...");
                    return;
                }
                
                // TODO: also need to consider latency. If we know the player actions ocurred ~50ms ago
                // which is ~5 physics updates, how do we reason about the current state?

                // immediate response mode:
                for (ComponentType i = 0; i < MAX_COMPONENT_TYPES; i++)
                {
                    if (updateData.sign.test(i))
                    {
                        // TODO: check if component exists locally? 
                        // If not we can implicitly add_component a default one (and then it will be updated)

                        std::string componentName = m_workingCosmos->get_component_name(i);

                        // TODO: switch on known component names:
                        // stream>> them myself, interrogate their values, and then overwrite the ecs

                        // default:
                        //m_workingCosmos->deserialize_and_write_component(updateEvent, componentName, updateData.id);
                    }
                }
            }
        }

    private:
        // inherits A_NetworkRelay::m_workingCosmos
        // inherits A_NetworkRelay::m_networkMessages
    };
}

#endif // SERVER_ENTITY_UPDATE_RELAY_H