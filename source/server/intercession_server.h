#ifndef INTERCESSION_SERVER_H
#define INTERCESSION_SERVER_H

//#include "intercession_pch.h"

#include "networking/net_i_server.h"
#include "events/event_types.h"

namespace pleep
{
namespace net
{
    class IntercessionServer : public I_Server<EventId>
    {
    public:
        // Accept an eventBroker pointer from NetworkDynamo to dispatch events to
        IntercessionServer(uint16_t port, EventBroker* eventBroker = nullptr)
            : I_Server<EventId>(port)
            , m_sharedBroker(eventBroker)
        {}

    protected:
        bool on_remote_connect(std::shared_ptr<Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("Checking new connection: " + remote->get_endpoint().address().to_string() + ":" + std::to_string(remote->get_endpoint().port()));
            return true;
        }
        
        void on_remote_validated(std::shared_ptr<Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Checking validated connection");

            PLEEPLOG_DEBUG("Sending appInfo message");
            Message<EventId> msg(events::network::APP_INFO);
            events::network::APP_INFO_params localInfo;
            msg << localInfo;
            remote->send(msg);
        }
        
        void on_remote_disconnect(std::shared_ptr<Connection<EventId>> remote) override
        {
            UNREFERENCED_PARAMETER(remote);
            PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Disconnected connection");
        }
        
        void on_message(std::shared_ptr<Connection<EventId>> remote, Message<EventId>& msg) override
        {
            switch (msg.header.id)
            {
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Bouncing intercession update message");
                // just bounce back
                remote->send(msg);
            }
            break;
            case events::network::ENTITY_UPDATE:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Received entity update message");
                // forward message over eventBroker (to network relays)
                m_sharedBroker->send_event(msg);

                // eventBroker will* be sending copies so we can unpack the message safely here
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("[" + std::to_string(remote->get_id()) + "] Recieved unknown message");
            }
            break;
            }
        }
        
        // We don't have direct access to internal cosmos, only the ability to broadcast messages back to the simulation systems (relays)
        EventBroker* m_sharedBroker;
    };
}
}

#endif // INTERCESSION_SERVER_H