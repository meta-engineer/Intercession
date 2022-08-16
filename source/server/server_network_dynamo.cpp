#include "server_network_dynamo.h"

namespace pleep
{
    ServerNetworkDynamo::ServerNetworkDynamo(EventBroker* sharedBroker)
        : I_NetworkDynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Setup Server Networking pipeline");
        // setup relays

        m_server = std::make_unique<net::IntercessionServer>(61336, m_sharedBroker);
        m_server->start();

        PLEEPLOG_TRACE("Done Server Networking pipeline setup");
    }
    
    ServerNetworkDynamo::~ServerNetworkDynamo() 
    {
        
    }
    
    void ServerNetworkDynamo::run_relays(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // handle incoming messages since last frame
        // events which pertain data outside of networking will be broadcast 
        m_server->process_received_messages();
        
    }
    
    void ServerNetworkDynamo::reset_relays() 
    {
        
    }
}