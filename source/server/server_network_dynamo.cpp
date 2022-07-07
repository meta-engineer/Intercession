#include "server_network_dynamo.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker)
        : A_Dynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Server Networking pipeline");
        // setup relays

        m_server = std::make_unique<net::PleepServer>(91339);
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
        //m_server->update();
        
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        
    }
}