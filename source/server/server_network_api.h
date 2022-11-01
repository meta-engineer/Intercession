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
            : I_Server<EventId>(port)
        {}

    protected:
        // ***** I_Server async callbacks *****

        bool on_remote_connect(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[----] Checking new connection: " + remote->get_endpoint().address().to_string() + ":" + std::to_string(remote->get_endpoint().port()));

            // check against some banned ips?

            return true;
        }
        
        void on_remote_validated(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Checking validated connection");

            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Responding with app info message");
            Message<EventId> msg(events::network::APP_INFO);
            events::network::APP_INFO_params localInfo;
            msg << localInfo;
            remote->send(msg);

            // TODO: signal "new player" event for dynamo on queue
            //m_incomingMessages.push_back();
        }
        
        void on_remote_disconnect(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Disconnected connection");

            // TODO: signal "player exit" event for dynamo on queue
            //m_incomingMessages.push_back();
        }
    };
}

#endif // SERVER_NETWORK_API_H