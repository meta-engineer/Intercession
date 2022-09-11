#ifndef A_DYNAMO_H
#define A_DYNAMO_H

//#include "intercession_pch.h"
#include "events/event_broker.h"

namespace pleep
{
    class A_Dynamo
    {
    protected:
        A_Dynamo(EventBroker* sharedBroker)
            : m_sharedBroker(sharedBroker)
        {
            // no guarentees sharedBroker isn't null
        }
    public:
        virtual ~A_Dynamo() = default;

        // universal pre-frame init for dynamos
        //virtual void prime();

        virtual void run_relays(double deltaTime) = 0;

        virtual void reset_relays() = 0;

        // synchros get broker reference from respective dynamo
        EventBroker* get_shared_broker()
        {
            return m_sharedBroker;
        }

    protected:
        // all dynamos receive broker from context on construction
        EventBroker* m_sharedBroker = nullptr;
    };
}

#endif // A_DYNAMO_H