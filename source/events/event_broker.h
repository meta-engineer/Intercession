#ifndef EVENT_BROKER_H
#define EVENT_BROKER_H

//#include "intercession_pch.h"
#include <unordered_map>
#include <list>

#include "event_types.h"
#include "event.h"

namespace pleep
{
    class EventBroker
    {
    public:
        void add_listener(EventId eventId, std::function<void(Event&)> const& listener);

        // synchronous send to all registered listeners
        void send_event(Event& event);
        void send_event(EventId eventId);

    private:
        // store subscriber's callbacks
        std::unordered_map<EventId, std::list<std::function<void(Event&)>>> m_listeners;
    };
}

#endif // EVENT_BROKER_H