#include "event_broker.h"

#include "logging/pleep_log.h"

namespace pleep
{
    void EventBroker::add_listener(EventId eventId, std::function<void(EventMessage&)> const& listener) 
    {
        // may be convenient to have a better eventId name conversion
        PLEEPLOG_TRACE("Add listener for event " + std::to_string(eventId));
        m_listeners[eventId].push_back(listener);
    }
    
    void EventBroker::remove_listener(EventId eventId, std::function<void(EventMessage&)> const& listener) 
    {
        PLEEPLOG_TRACE("Remove listener for event " + std::to_string(eventId));
        m_listeners[eventId].remove_if([listener](std::function<void(EventMessage&)> f){ return f.target_type() == listener.target_type(); });
    }
    
    void EventBroker::send_event(EventMessage& event) 
    {
        EventId type = event.header.id;
        PLEEPLOG_TRACE("Triggering callbacks for event " + std::to_string(type));

        for (std::function<void(EventMessage&)> const& listener : m_listeners[type])
        {
            listener(event);
        }
    }
    
    void EventBroker::send_event(EventId eventId) 
    {
        PLEEPLOG_TRACE("Triggering callbacks for event " + std::to_string(eventId));
        // construct parameter-less event of given id
        EventMessage event(eventId);

        for (std::function<void(EventMessage&)> const& listener : m_listeners[eventId])
        {
            listener(event);
        }
    }
}