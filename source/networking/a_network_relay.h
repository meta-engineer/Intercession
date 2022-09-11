#ifndef A_NETWORK_RELAY_H
#define A_NETWORK_RELAY_H

//#include "intercession_pch.h"
#include "core/cosmos_access_packet.h"
#include "events/event_broker.h"

namespace pleep
{
    class A_NetworkRelay
    {
    protected:
        A_NetworkRelay() = default;
    public:
        virtual ~A_NetworkRelay() = default;

        // Use same submit/engage/clear relay paradigm
        // but only submittion will be a CosmosAccessPacket
        // and other data will be "submitted" from events called by a network interface
        // when it recieves the corresponding network message

        // network relays should not need specific entities to be submitted
        // and should only need to access sparse entities in Cosmos
        void submit(CosmosAccessPacket data)
        {
            m_workingCosmos = data.owner;
        }

        // Do we even use deltaTime?
        virtual void engage(double deltaTime) = 0;

        // clear must have default behaviour to dispose cosmos reference
        // but is overridable (remember to call A_NetworkRelay::clear()!!!)
        virtual void clear()
        {
            m_workingCosmos = nullptr;
        }

    protected:
        // only store one loose (unowned) cosmos reference
        Cosmos* m_workingCosmos = nullptr;

        // receive broker from dynamo on construction
        EventBroker* m_sharedBroker = nullptr;
    };
}

#endif // A_NETWORK_RELAY_H