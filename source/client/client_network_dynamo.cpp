#include "client_network_dynamo.h"

#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

#include "staging/client_focal_entity.h"
#include "staging/client_local_entities.h"

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
        m_sharedBroker->add_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ClientNetworkDynamo::_jump_request_handler));
        
        PLEEPLOG_TRACE("Done client networking pipeline setup");
    }

    ClientNetworkDynamo::~ClientNetworkDynamo() 
    {
        // clear event handlers
        m_sharedBroker->remove_listener(METHOD_LISTENER(events::network::JUMP_REQUEST, ClientNetworkDynamo::_jump_request_handler));
    }

    void ClientNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        if (!m_networkApi.is_ready()) return;

        std::shared_ptr<Cosmos> cosmos = m_workingCosmos.expired() ? nullptr : m_workingCosmos.lock();

        if (!cosmos)
        {
            //PLEEPLOG_WARN("Relays cannot do any meaningful work with a null Cosmos");
            // don't return so that we can actively clear out incoming messages
        }

        // before receiving any entity updates, send our focal entity state
        if (cosmos)
        {
            Entity focalEntity = cosmos->get_focal_entity();
            if (focalEntity != NULL_ENTITY)
            {
                EventMessage focalUpdate(events::cosmos::ENTITY_UPDATE);
                events::cosmos::ENTITY_UPDATE_params focalInfo = {
                    focalEntity, 
                    cosmos->get_entity_signature(focalEntity) & cosmos->get_category_signature(ComponentCategory::upstream),
                    true
                };
                cosmos->serialize_entity_components(focalInfo.entity, focalInfo.sign, focalUpdate);
                focalUpdate << focalInfo;
                m_networkApi.send_message(focalUpdate);
            }
        }

        // Handling incoming server messages from network
        //inline _process_network_messages()
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        Message<EventId> msg;
        while ((messageCount++) < maxMessages && m_networkApi.pop_message(msg))
        {
            if (!cosmos) continue;
            
            switch (msg.header.id)
            {
            case events::network::PROGRAM_INFO:
            {
                PLEEPLOG_TRACE("Received PROGRAM_INFO message");
                // this message is received when connection succeeds

                // check program infos match
                events::network::PROGRAM_INFO_params localInfo;
                events::network::PROGRAM_INFO_params remoteInfo;
                msg >> remoteInfo;

                if (!(localInfo == remoteInfo))
                {
                    PLEEPLOG_WARN("Remote appInfo DOES NOT match local appInfo! I should consider disconnecting.");
                    // In future we can reduce this to only check compadibility not exact match
                    assert(localInfo == remoteInfo);
                    return;
                }

                PLEEPLOG_INFO("Remote appInfo matches local appInfo! Good to keep communicating.");

                EventMessage newClientMsg(events::network::NEW_CLIENT);
                events::network::NEW_CLIENT_params newClientInfo{
                    m_transferCode // 0 if we are a brand-new client connecting
                };
                newClientMsg << newClientInfo;
                m_networkApi.send_message(newClientMsg);

                // only use transfer code once
                m_transferCode = 0;
            }
            break;
            case events::cosmos::ENTITY_UPDATE:
            {
                events::cosmos::ENTITY_UPDATE_params updateInfo;
                msg >> updateInfo;
                //PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());

                // ensure entity exists
                if (cosmos->entity_exists(updateInfo.entity))
                {
                    // read update into Cosmos
                    cosmos->deserialize_entity_components(updateInfo.entity, updateInfo.sign, updateInfo.subset, msg);
                }
                else
                {
                    PLEEPLOG_ERROR("Received ENTITY_UPDATE for entity " + std::to_string(updateInfo.entity) + " which does not exist, skipping...");
                }
            }
            break;
            case events::cosmos::ENTITY_CREATED:
            {
                // register entity and create components to match signature
                events::cosmos::ENTITY_CREATED_params createInfo;
                msg >> createInfo;
                PLEEPLOG_DEBUG("Create Entity: " + std::to_string(createInfo.entity) + " | " + createInfo.sign.to_string());

                if (cosmos->register_entity(createInfo.entity))
                {
                    for (ComponentType c = 0; c < createInfo.sign.size(); c++)
                    {
                        if (createInfo.sign.test(c)) cosmos->add_component(createInfo.entity, c);
                    }
                }
            }
            break;
            case events::cosmos::ENTITY_REMOVED:
            {
                // entity deletion should be idempotent, incase our local simulation also deletes
                events::cosmos::ENTITY_REMOVED_params removeInfo;
                msg >> removeInfo;
                PLEEPLOG_TRACE("Remove Entity: " + std::to_string(removeInfo.entity));

                // use condemn event to avoid double deletion
                cosmos->condemn_entity(removeInfo.entity);
            }
            break;
            case events::network::SUPERPOSITION:
            {
                PLEEPLOG_TRACE("Received SUPERPOSITION message");
            }
            break;
            case events::network::NEW_CLIENT:
            {
                PLEEPLOG_TRACE("Received NEW_CLIENT message");
                // Server has forwarded our connection request back to us to confirm
                events::network::NEW_CLIENT_params clientInfo;
                msg >> clientInfo;

                PLEEPLOG_DEBUG("My assigned entity is " + std::to_string(clientInfo.entity));
                // any need to have an event? Does anyone other than the cosmos need to know?
                cosmos->set_focal_entity(clientInfo.entity);
                
                // create client side entities
                // only entities which the server doesn't need for simulation
                // will broadcast SET_MAIN_CAMERA
                // will check for cosmos' focal entity
                create_client_local_entities(cosmos, m_sharedBroker);

                // sync coherency
                cosmos->set_coherency(msg.header.coherency);
            }
            break;
            case events::network::COHERENCY_SYNC:
            {
                events::network::COHERENCY_SYNC_params syncInfo;
                msg >> syncInfo;

                cosmos->set_coherency(msg.header.coherency);
            }
            break;
            case events::network::JUMP_RESPONSE:
            {
                events::network::JUMP_RESPONSE_params jumpInfo;
                msg >> jumpInfo;

                // check if server has denied our jump request
                if (jumpInfo.port == 0) break;
                
                PLEEPLOG_TRACE("Received JUMP_RESPONSE message, i can connect to: " + std::to_string(jumpInfo.port) + " with trasfer code: " + std::to_string(jumpInfo.transferCode));

                // signal to clear the cosmos and prep for data from the new connection
                EventMessage condemnMsg(events::cosmos::CONDEMN_ALL);
                m_sharedBroker->send_event(condemnMsg);

                // TODO: I should know the currently connected address from startup
                restart_connection("127.0.0.1", jumpInfo.port);

                // save transfer code to use when connection succeeds
                m_transferCode = jumpInfo.transferCode;
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
        m_workingCosmos.reset();
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

    
    void ClientNetworkDynamo::restart_connection(const std::string& address, uint16_t port)
    {
        m_networkApi.disconnect();
        m_networkApi.connect(address, port);
    }

    void ClientNetworkDynamo::_jump_request_handler(EventMessage jumpMsg)
    {
        // some behavior has called to request a timeslice jump
        // forward message on to server, expecting a JUMP_RESPONSE sometime soon
        m_networkApi.send_message(jumpMsg);
    }
}