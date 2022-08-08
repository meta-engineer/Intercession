#include "client_network_dynamo.h"

#include "logging/pleep_log.h"

namespace pleep
{
    ClientNetworkDynamo::ClientNetworkDynamo(EventBroker* sharedBroker) 
        : I_NetworkDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Client Networking pipeline");
        // setup relays

        m_client = std::make_unique<net::PleepClient>();
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
        Message<net::PleepMessageType> msg;
        msg.header.id = net::PleepMessageType::intercession;
        char pleep[10] = "pleep";
        msg << pleep;
        PLEEPLOG_DEBUG("Sending intercession message");
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
        
        // handle incoming messages manually
        // TODO: should client have on_message callbacks? Which is easier to hook into the simulation and provide access to entities?
        if (m_client->is_connected())
        {
            if (!m_client->get_incoming_messages().empty())
            {
                auto msg = m_client->get_incoming_messages().pop_front().first.msg;

                switch (msg.header.id)
                {
                case net::PleepMessageType::entityUpdate:
                {
                    PLEEPLOG_DEBUG("Recieved entityUpdate message");
                    break;
                }
                case net::PleepMessageType::intercession:
                {
                    PLEEPLOG_DEBUG("Recieved intercession message");
                    char msgString[10];
                    msg >> msgString;
                    PLEEPLOG_DEBUG("Message body: " + std::string(msgString));
                    break;
                }
                default:
                {
                    PLEEPLOG_DEBUG("Recieved unknown message");
                    break;
                }
                }
            }
        }

    }

    void ClientNetworkDynamo::reset_relays() 
    {
        
    }
    
    void ClientNetworkDynamo::_entity_modified_handler(EventMessage entityEvent)
    {
        events::cosmos::ENTITY_MODIFIED_params entityData;
        entityEvent >> entityData;

        // TODO: store this entityId for our run_relays call later in the frame
        //entityData.entityId;
    }
}