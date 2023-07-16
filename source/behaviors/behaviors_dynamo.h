#ifndef BEHAVIORS_DYNAMO_H
#define BEHAVIORS_DYNAMO_H

//#include "intercession_pch.h"
#include <vector>

#include "core/a_dynamo.h"
#include "behaviors/behaviors_packet.h"

namespace pleep
{
    // Invokes the periodic (fixed & frame) behaviors callbacks
    // Provides them with resource access
    class BehaviorsDynamo : public A_Dynamo
    {
    public:
        BehaviorsDynamo(EventBroker* sharedBroker);
        ~BehaviorsDynamo();

        void submit(BehaviorsPacket data);

        void run_relays(double deltaTime) override;

        void reset_relays() override;

    private:
        // relay function can be inline in the dynamo,
        // I cant think of functions that we'd need relay modularity for...
        // so we'll store submitted packets ourselves
        std::vector<BehaviorsPacket> m_behaviorsPackets;
    };
}

#endif // BEHAVIORS_DYNAMO_H