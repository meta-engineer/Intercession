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

        m_client = std::make_unique<net::IntercessionClient>(m_sharedBroker);
        m_client->connect("127.0.0.1", 61336);

        // TESTING
        // wait until connection is ready (max timeout 1 sec)
        std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        std::chrono::duration<double> deltaTime;
        while(!m_client->is_ready() && deltaTime < std::chrono::seconds(1))
        {
            deltaTime = std::chrono::system_clock::now() - startTime;
        }
        PLEEPLOG_DEBUG("Waited " + std::to_string(deltaTime.count()) + " seconds for client to connect");
        Message<EventId> msg;
        msg.header.id = events::network::INTERCESSION_UPDATE;
        char pleep[10] = "pleep";
        msg << pleep;
        PLEEPLOG_DEBUG("Sending intercession update message");
        m_client->send_message(msg);


        m_sharedBroker->add_listener(METHOD_LISTENER(events::cosmos::ENTITY_MODIFIED, ClientNetworkDynamo::_entity_modified_handler) );
        
        PLEEPLOG_TRACE("Done Client Networking pipeline setup");
    }

    ClientNetworkDynamo::~ClientNetworkDynamo() 
    {
    }

    void ClientNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        
        // handle incoming messages since last frame
        // events which pertain data outside of networking will be broadcast 
        m_client->process_received_messages();

        // TODO: move this to a relay?
        // TODO: What to do if our connection is gone?
        if (m_client->is_connected())
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

                    m_workingCosmos->serialize_entity_components(ent, entMsg);
                    PLEEPLOG_DEBUG("Finished component serialization");
                    // fetch timeline id and pack header manually
                    std::pair<TemporalEntity, CausalChainLink> timelineId; // TODO
                    events::network::ENTITY_UPDATE_params entUpdate = { timelineId.first, timelineId.second, entSign };
                    entMsg << entUpdate;
                    PLEEPLOG_DEBUG("Finished header serialization");

                    PLEEPLOG_DEBUG("Sending entity update message for TemporalEntity: " + std::to_string(entUpdate.id) + ", link: " + std::to_string(entUpdate.link));

                    // send update message
                    m_client->send_message(entMsg);
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
        // save Cosmos reference until end of frame
        m_workingCosmos = data.owner;
    }
    
    void ClientNetworkDynamo::_entity_modified_handler(EventMessage entityEvent)
    {
        events::cosmos::ENTITY_MODIFIED_params entityData;
        entityEvent >> entityData;

        // maintain uniqueness
        if (m_entitiesToReport.find(entityData.entityId) == m_entitiesToReport.end())
        {
            m_entitiesToReport.insert(entityData.entityId);
        }
    }
}