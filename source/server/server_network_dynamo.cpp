#include "server_network_dynamo.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker)
        : A_Dynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Server Networking pipeline");
        // setup relays

        m_server = std::make_unique<net::PleepServer>(61336);
        m_server->start();

        PLEEPLOG_TRACE("Done Server Networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        
    }
    
    void ServerNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // handle all messages received between frames
        m_server->process_received_messages();
        
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        
    }
}