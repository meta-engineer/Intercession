#include "client_network_dynamo.h"

#include "logging/pleep_log.h"

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

                    // pack in reverse order, so receiver will read FILO
                    // Signature size is defined as a ComponentType so no loss is possible
                    assert(entSign.size() == MAX_COMPONENT_TYPES);
                    // ComponentType cannot assign to -1, it will underflow to 255
                    for (ComponentType i = static_cast<ComponentType>(entSign.size()) - 1; i < MAX_COMPONENT_TYPES; i--)
                    {
                        if (entSign.test(i))
                        {
                            // this index is valid
                            // get component typename from index (component Id)
                            std::string componentTypename(m_workingCosmos->get_component_name(i));
                            //PLEEPLOG_DEBUG("Found component to serialize: " + componentTypename);

                            // switch on all "communicable" components? feed them into entMsg
                            // components not explicitly listed can be either ignored, or dispatched to ECS to be added to message anyway?
                            // maybe all components should dispatch to be filled by ECS...
                            // then ECS needs to have message serialization methods
                        }
                    }
                    events::network::ENTITY_UPDATE_params entUpdate = { ent, entSign };
                    entMsg << entUpdate;

                    // send update message
                    m_client->send_message(entMsg);
                }
            }
        }
    }

    void ClientNetworkDynamo::reset_relays() 
    {
        //clear cosmos reference each frame
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

        m_entitiesToReport.insert(entityData.entityId);
    }
}