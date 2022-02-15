#include "i_dynamo.h"

namespace pleep
{
    IDynamo::IDynamo(EventBroker* sharedBroker)
        : m_sharedBroker(sharedBroker)
    {
        // no guarentees sharedBroker isn't null
    }
    
    EventBroker* IDynamo::get_shared_broker() 
    {
        return m_sharedBroker;
    }
    
}