#include "client_network_dynamo.h"

#include "logging/pleep_log.h"
#include "networking/timeline_types.h"

namespace pleep
{
    ClientNetworkDynamo::ClientNetworkDynamo(EventBroker* sharedBroker) 
        : I_NetworkDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Client Networking pipeline");
        // setup relays

        m_networkApi.connect("127.0.0.1", 61336);

        // *** TESTING ***
        // wait until connection is ready (max timeout 1 sec)
        std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        std::chrono::duration<double> deltaTime;
        while(!m_networkApi.is_ready() && deltaTime < std::chrono::seconds(1))
        {
            deltaTime = std::chrono::system_clock::now() - startTime;
        }
        PLEEPLOG_DEBUG("Waited " + std::to_string(deltaTime.count()) + " seconds for client to connect");
        Message<EventId> msg;
        // for serialization testing server will reply with a new serialized entity
        msg.header.id = events::network::INTERCESSION_UPDATE;
        char pleep[10] = "pleep";
        msg << pleep;
        PLEEPLOG_DEBUG("Sending intercession update message");
        m_networkApi.send_message(msg);
        // *** TESTING ***

        // setup handlers
        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_MODIFIED, ClientNetworkDynamo::_entity_modified_handler) );
        
        PLEEPLOG_TRACE("Done Client Networking pipeline setup");
    }

    ClientNetworkDynamo::~ClientNetworkDynamo() 
    {
    }

    void ClientNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);

        // Handling incoming server messages from network
        //_process_network_messages();
        // aka
        const size_t maxMessages = static_cast<size_t>(-1);
        size_t messageCount = 0;
        while ((messageCount++) < maxMessages && m_networkApi.is_message_available())
        {
            Message<EventId> msg = m_networkApi.pop_message();
            switch (msg.header.id)
            {
            case events::network::APP_INFO:
            {
                PLEEPLOG_DEBUG("Recieved appInfo message");
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
            case events::network::ENTITY_UPDATE:
            {
                PLEEPLOG_DEBUG("Recieved entityUpdate message");
            }
            break;
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_DEBUG("Recieved intercessionUpdate message");
                char msgString[10];
                msg >> msgString;
                PLEEPLOG_DEBUG("Message body: " + std::string(msgString));
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Recieved unknown message");
            }
            break;
            }
        }

        // report entities modified this frame
        // TODO: move this to a relay
        if (m_networkApi.is_connected())
        {
            // package entities to report as events::network::ENTITY_UPDATE
            if (m_workingCosmos)
            {
                for (Entity ent : m_entitiesToReport)
                {
                    Message<EventId> entMsg(events::network::ENTITY_UPDATE);
                    Signature entSign = m_workingCosmos->get_entity_signature(ent);
                    PLEEPLOG_DEBUG("Found modified entity to report: " + std::to_string(ent) + " (" + entSign.to_string() + ")");

                    // Interrogate any components now before serializing!

                    // fetch timeline id
                    std::pair<TemporalEntity, CausalChainLink> timelineId = m_workingCosmos->get_temporal_identifier(ent);
                    if (timelineId.first == NULL_TEMPORAL_ENTITY)
                    {
                        PLEEPLOG_WARN("Tried to report local entity (" + std::to_string(ent) + ") which does not match a TemporalEntity, skipping...");
                        continue;
                    }

                    m_workingCosmos->serialize_entity_components(ent, entMsg);
                    PLEEPLOG_DEBUG("Finished component serialization");
                    // pack header (params) manually
                    events::network::ENTITY_UPDATE_params entUpdate = { timelineId.first, timelineId.second, entSign };
                    entMsg << entUpdate;
                    PLEEPLOG_DEBUG("Finished header serialization");

                    PLEEPLOG_DEBUG("Sending entity update message for TemporalEntity: " + std::to_string(entUpdate.id) + ", link: " + std::to_string(entUpdate.link));

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
        m_entitiesToReport.insert(entityData.id);
    }
}