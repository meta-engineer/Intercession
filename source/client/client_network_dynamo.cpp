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
        // Recieve this address from? User input?
        m_networkApi.connect("127.0.0.1", 61336);
        // Server will send message to acknowledge connection is ready
        // Keep frame loop alive until then

        // setup handlers
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_MODIFIED, ClientNetworkDynamo::_entity_modified_handler) );
        
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
                PLEEPLOG_TRACE("Recieved appInfo message");
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
                //PLEEPLOG_TRACE("Recieved entityUpdate message");

                events::cosmos::ENTITY_UPDATE_params updateInfo;
                msg >> updateInfo;
                //PLEEPLOG_DEBUG(std::to_string(updateInfo.entity) + " | " + updateInfo.sign.to_string());
                PLEEPLOG_DEBUG("Update Entity: " + std::to_string(updateInfo.entity));

                // confirm entity exists?
                // confirm entity signatures match?

                // read update into Cosmos
                // This assumes entity's current signature is correct!
                m_workingCosmos->deserialize_entity_components(updateInfo.entity, msg);
            }
            break;
            case events::cosmos::ENTITY_CREATED:
            {
                PLEEPLOG_TRACE("Recieved entityCreated message");

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
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_TRACE("Recieved intercessionUpdate message");
            }
            break;
            break;
            case events::network::NEW_CLIENT:
            {
                PLEEPLOG_TRACE("Recieved newClient message");
                // Server has forwarded our connection request back to us to confirm

                build_pc(m_workingCosmos);

                // send request to server to validate our new entity
            }
            default:
            {
                PLEEPLOG_TRACE("Recieved unknown message: " + msg.info());
            }
            break;
            }
        }


        // TODO: move this to a relay, EntityUpdateRelay?
        // Should relays accumulate "packets" via events signalled from other subsystems
        //     (like ENTITY_MODIFIED)
        // and then in run we can send messages over the network all at once?

        // report entities modified this frame
        if (m_networkApi.is_connected())
        {
            // package entities to report as events::cosmos::ENTITY_UPDATE
            if (m_workingCosmos)
            {
                for (Entity ent : m_entitiesToReport)
                {
                    Message<EventId> entMsg(events::cosmos::ENTITY_UPDATE);
                    Signature entSign = m_workingCosmos->get_entity_signature(ent);
                    PLEEPLOG_DEBUG("Found modified entity to report: " + std::to_string(ent) + " (" + entSign.to_string() + ")");

                    // Interrogate any components now before serializing!

                    // fetch timeline id
                    TimesliceId entTimeslice = derive_timeslice_id(ent);
                    if (entTimeslice == NULL_TIMESLICEID)
                    {
                        PLEEPLOG_WARN("Tried to report local entity (" + std::to_string(ent) + ") which does not match a TemporalEntity, skipping...");
                        continue;
                    }

                    m_workingCosmos->serialize_entity_components(ent, entMsg);
                    PLEEPLOG_DEBUG("Finished component serialization");
                    // pack header (params) manually
                    events::cosmos::ENTITY_UPDATE_params entUpdate = { ent, entSign };
                    entMsg << entUpdate;
                    PLEEPLOG_DEBUG("Finished header serialization");

                    PLEEPLOG_DEBUG("Sending entity update message for Entity: " + std::to_string(ent));

                    // send update message
                    m_networkApi.send_message(entMsg);
                }
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
    
    void ClientNetworkDynamo::_entity_modified_handler(EventMessage entityEvent)
    {
        events::cosmos::ENTITY_MODIFIED_params entityData;
        entityEvent >> entityData;

        // insert maintain uniqueness
        m_entitiesToReport.insert(entityData.entity);
    }
}