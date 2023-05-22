#ifndef SERVER_NETWORK_API_H
#define SERVER_NETWORK_API_H

//#include "intercession_pch.h"
#include "networking/net_i_server.h"
#include "events/event_types.h"

namespace pleep
{
    // Implement a template specific I_Server
    // provide the required async callbacks to manage any server specific bookkeeping
    // and convert them to template specific events in the incoming message queue
    // to consolidate all changes for user to ingest with I_Server::pop_message
    class ServerNetworkApi : public net::I_Server<EventId>
    {
    public:
        ServerNetworkApi(uint16_t port)
            : net::I_Server<EventId>(port)
        {}

    protected:
        // ***** I_Server async callbacks *****

        bool on_remote_connect(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[----] Checking new connection: " + remote->get_endpoint().address().to_string() + ":" + std::to_string(remote->get_endpoint().port()));

            // check against some banned ips?
            //return false;

            return true;
        }
        
        void on_remote_validated(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Checking validated connection");

            // "forward" new client notice in lieu of client sending it (it is implied when client calls to connect)
            net::OwnedMessage<EventId> msg;
            msg.msg = EventMessage(events::network::NEW_CLIENT);
            msg.remote = remote;
            events::network::NEW_CLIENT_params clientInfo;  // is this necessary?
            msg.msg << clientInfo;
            m_incomingMessages.push_back(msg);
        }
        
        void on_remote_disconnect(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Disconnected connection");

            // TODO: signal "player exit" event for dynamo on queue
            // TODO: Check if remote shared_pointer beyond this method if we pass it to network dynamo
            //m_incomingMessages.push_back();
        }
    };
}

#endif // SERVER_NETWORK_API_H