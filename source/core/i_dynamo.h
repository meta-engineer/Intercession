#ifndef I_DYNAMO_H
#define I_DYNAMO_H

//#include "intercession_pch.h"
#include "events/event_broker.h"

namespace pleep
{
    class IDynamo
    {
    public:
        IDynamo(EventBroker* sharedBroker);
        // universal pre-frame init for dynamos
        virtual void prime();
        virtual void run_relays(double deltaTime) = 0;

        // synchros get broker reference from respective dynamo
        EventBroker* get_shared_broker();

    protected:
        EventBroker* m_sharedBroker;
    };
}

#endif // I_DYNAMO_H