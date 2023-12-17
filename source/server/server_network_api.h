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
            PLEEPLOG_INFO("[----] Checking new connection: " + remote->get_endpoint().address().to_string() + ":" + std::to_string(remote->get_endpoint().port()));

            // check against some banned ips?
            //return false;

            return true;
        }
        
        void on_remote_validated(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_INFO("[" + std::to_string(remote->get_id()) + "] Checking validated connection");

            // New Connection Could be BRAND-new client, or could be a client transferring from another timeslice
            // So we have to wait for them to notify us
            // For safety this should be a 1 time notification, once they initalize their type, we respond accordingly and then they cannot change it.

            // send program info to client immediately from here (as an ACK), then client can send NEW_CLIENT msg to normal network message queue

            EventMessage appMessage(events::network::PROGRAM_INFO);
            events::network::PROGRAM_INFO_params appInfo{};
            appMessage << appInfo;
            remote->send(appMessage);
            // Don't send any frame updates to this connection until it is initialized
            remote->disable_sending();
        }
        
        void on_remote_disconnect(std::shared_ptr<net::Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_INFO("[" + std::to_string(remote->get_id()) + "] Disconnected connection");

            // TODO: signal "player exit" event for dynamo on queue
            // TODO: Check if remote shared_pointer beyond this method if we pass it to network dynamo
            //m_incomingMessages.push_back();
        }
    };
}

#endif // SERVER_NETWORK_API_H