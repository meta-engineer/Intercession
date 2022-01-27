#include "event_broker.h"

#include "logging/pleep_log.h"

namespace pleep
{
    void EventBroker::add_listener(EventId eventId, std::function<void(Event&)> const& listener) 
    {
        // may be convenient to have a better eventId name conversion
        PLEEPLOG_TRACE("Add listener triggered for event " + std::to_string(eventId));
        m_listeners[eventId].push_back(listener);
    }
    
    void EventBroker::send_event(Event& event) 
    {
        EventId type = event.get_type();
        PLEEPLOG_TRACE("Triggering callbacks for event " + std::to_string(type));

        for (std::function<void(Event&)> const& listener : m_listeners[type])
        {
            listener(event);
        }
    }
    
    void EventBroker::send_event(EventId eventId) 
    {
        PLEEPLOG_TRACE("Triggering callbacks for event " + std::to_string(eventId));
        // construct parameter-less event of given id
        Event event(eventId);

        for (std::function<void(Event&)> const& listener : m_listeners[eventId])
        {
            listener(event);
        }
    }
}