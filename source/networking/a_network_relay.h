#ifndef A_NETWORK_RELAY_H
#define A_NETWORK_RELAY_H

//#include "intercession_pch.h"
#include <vector>

#include "core/cosmos_access_packet.h"
#include "events/event_types.h"

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

        // Accept network message dispatched by dynamo
        // type is not strict so relays will have to check header id
        void submit(EventMessage& msg)
        {
            m_networkMessages.push_back(msg);
        }

        // Do we even use deltaTime?
        virtual void engage(double deltaTime) = 0;

        // clear must have default behaviour to dispose cosmos reference
        // but is overridable (remember to call A_NetworkRelay::clear()!!!)
        virtual void clear()
        {
            m_workingCosmos.reset();
            m_networkMessages.clear();
        }

    protected:
        // only store one loose (unowned) cosmos reference
        std::weak_ptr<Cosmos> m_workingCosmos;
        
        // collect packets during message submitting
        std::vector<EventMessage> m_networkMessages;
    };
}

#endif // A_NETWORK_RELAY_H