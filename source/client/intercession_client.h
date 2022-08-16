#ifndef INTERCESSION_CLIENT_H
#define INTERCESSION_CLIENT_H

//#include "intercession_pch.h"
#include "networking/net_i_client.h"
#include "events/event_types.h"

namespace pleep
{
namespace net
{
    class IntercessionClient : public I_Client<EventId>
    {
    public:
        // Accept an eventBroker pointer from NetworkDynamo to dispatch events to
        // Connection is made after construction with I_Client::connect()
        IntercessionClient(EventBroker* eventBroker = nullptr)
            : I_Client<EventId>()
            , m_sharedBroker(eventBroker)
        {}

    protected:
        void on_message(Message<EventId>& msg) override
        {
            switch (msg.header.id)
            {
            case events::network::APP_INFO:
            {
                PLEEPLOG_DEBUG("Recieved appInfo message");
                events::network::APP_INFO_params localInfo;
                events::network::APP_INFO_params remoteInfo;
                msg >> remoteInfo;

                if (localInfo == remoteInfo)
                {
                    PLEEPLOG_DEBUG("Remote appInfo matches local appInfo! Good to keep communicating.");
                }
                else
                {
                    PLEEPLOG_DEBUG("Remote appInfo DOES NOT match local appInfo! I should consider disconnecting.");
                }
            }
            break;
            case events::network::ENTITY_UPDATE:
            {
                PLEEPLOG_DEBUG("Recieved entityUpdate message");
            }
            break;
            case events::network::INTERCESSION_UPDATE:
            {
                PLEEPLOG_DEBUG("Recieved intercessionUpdate message");
                char msgString[10];
                msg >> msgString;
                PLEEPLOG_DEBUG("Message body: " + std::string(msgString));
            }
            break;
            default:
            {
                PLEEPLOG_DEBUG("Recieved unknown message");
            }
            break;
            }
        }
        
        // We don't have direct access to internal cosmos, only the ability to broadcast messages back to the simulation systems (relays)
        EventBroker* m_sharedBroker;
    };
}
}

#endif // INTERCESSION_CLIENT_H