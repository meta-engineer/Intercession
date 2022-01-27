#include "event.h"

namespace pleep
{
    Event::Event(EventId type)
        : m_type(type)
    {}

    EventId Event::get_type() const
    {
        return m_type;
    }
}