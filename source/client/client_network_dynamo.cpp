#include "client_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

#include "staging/build_pc.h"

namespace pleep
{
    ClientNetworkDynamo::ClientNetworkDynamo(EventBroker* sharedBroker) 
        : I_NetworkDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Start client networking pipeline setup");
        // setup relays

        // TEMP: direct connect to known address
        // Receive this address from? User input?
        m_networkApi.connect("127.0.0.1", 61336);
        // Server will send message to acknowledge connection is ready
        // Keep frame loop alive until then

        // setup any event handlers
        
        PLEEPLOG_TRACE("Done client networking pipeline setup");
    }

    ClientNetworkDynamo::~ClientNetworkDynamo() 
    {
    }

    void ClientNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        // Handling incoming server messages from network
        //inline _process_network_messages()
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        while ((messageCount++) < maxMessages && m_networkApi.is_message_available())
        {
            Message<EventId> msg = m_networkApi.pop_message();
            switch (msg.header.id)
            {
            case events::network::APP_INFO:
            {
                PLEEPLOG_TRACE("Received APP_INFO message");
                events::network::APP_INFO_params localInfo;
                events::network::APP_INFO_params remoteInfo;
                msg >> remoteInfo;

                if (localInfo == remoteInfo)
                {
                    PLEEPLOG_DEBUG("Remote appInfo matches local appInfo! Good to keep communicating.");
                    PLEEPLOG_DEBUG("I will respond with my INTERCESSION_APP_INFO");
                    Message<EventId> response(events::network::INTERCESSION_APP_INFO);
                    events::network::INTERCESSION_APP_INFO_params localIntercessionInfo;
                    response << localIntercessionInfo;

                    //m_networkApi.send_message(response);
                }
                else
                {
                    PLEEPLOG_DEBUG("Remote appInfo DOES NOT match local appInfo! I should consider disconnecting.");
                }
            }
            break;
            case events::cosmos::ENTITY_UPDATE:
            {
                //PLEEPLOG_TRACE("Received ENTITY_UPDATE message");

                events::cosmos::ENTITY_UPDATE_params updateInfo;
                msg >> updateInfo;
                //PLEEPLOG_DEBUG(std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());
                //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity));

                // confirm entity exists?
                // confirm entity signatures match?

                // read update into Cosmos
                // This assumes entity's current signature is correct!
                m_workingCosmos->deserialize_entity_components(updateInfo.entity, msg);
            }
            break;
            case events::cosmos::ENTITY_CREATED:
            {
                PLEEPLOG_TRACE("Received ENTITY_CREATED message");

                // register entity and create components to match signature
                events::cosmos::ENTITY_CREATED_params createInfo;
                msg >> createInfo;
                PLEEPLOG_DEBUG(std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                if (m_workingCosmos->register_entity(createInfo.entity))
                {
                    for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                    {
                        if (createInfo.sign.test(c)) m_workingCosmos->add_component(createInfo.entity, c);
                    }
                }
            }
            break;
            case events::cosmos::ENTITY_MODIFIED:
            {
                PLEEPLOG_TRACE("Received ENTITY_MODIFIED message");

                // create any new components, remove any components message does not have
                events::cosmos::ENTITY_MODIFIED_params modInfo;
                msg >> modInfo;
                PLEEPLOG_DEBUG(std::to_string(modInfo.entity) + " | " + modInfo.sign.to_string());

                // ensure entity exists
                if (m_workingCosmos->entity_exists(modInfo.entity))
                {
                    Signature entitySign = m_workingCosmos->get_entity_signature(modInfo.entity);

                    for (ComponentType c = 0; c < modInfo.sign.size(); c++)
                    {
                        if (modInfo.sign.test(c) && !entitySign.test(c)) m_workingCosmos->add_component(modInfo.entity, c);

                        if (!modInfo.sign.test(c) && entitySign.test(c)) m_workingCosmos->remove_component(modInfo.entity, c);
                    }
                }

            }
            break;
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_TRACE("Received INTERCESSION_UPDATE message");
            }
            break;
            case events::network::NEW_CLIENT:
            {
                PLEEPLOG_TRACE("Received NEW_CLIENT message");
                // Server has forwarded our connection request back to us to confirm
                events::network::NEW_CLIENT_params clientInfo;
                msg >> clientInfo;

                PLEEPLOG_DEBUG("My assigned entity is " + std::to_string(clientInfo.entity));

                // create camera, abstract input entity?
                // only entities which the server doesn't need for simulation
                // maybe it wants out input entity also then?

            }
            break;
            default:
            {
                PLEEPLOG_TRACE("Received unknown message: " + msg.info());
            }
            break;
            }
        }
    }

    void ClientNetworkDynamo::reset_relays() 
    {
        // clear cosmos reference each frame
        m_workingCosmos = nullptr;
    }
    
    void ClientNetworkDynamo::submit(CosmosAccessPacket data) 
    {
        // TEMP: save Cosmos reference until end of frame
        m_workingCosmos = data.owner;
        // TODO: pass this to relay
    }

    
    size_t ClientNetworkDynamo::get_num_connections()
    {
        return m_networkApi.is_ready();
    }
}