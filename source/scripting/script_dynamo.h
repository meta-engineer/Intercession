#ifndef SCRIPT_DYNAMO_H
#define SCRIPT_DYNAMO_H

//#include "intercession_pch.h"
#include <vector>

#include "core/a_dynamo.h"
#include "scripting/script_packet.h"

namespace pleep
{
    // Invokes the periodic (fixed & frame) script callbacks
    // Provides them with resource access
    class ScriptDynamo : public A_Dynamo
    {
    public:
        ScriptDynamo(EventBroker* sharedBroker);
        ~ScriptDynamo();

        void submit(ScriptPacket data);

        void run_relays(double deltaTime) override;

        void reset_relays() override;

    private:
        // relay function can be inline in the dynamo,
        // I cant think of functions that we'd need relay modularity for...
        // so we'll store submitted packets ourselves
        std::vector<ScriptPacket> m_scriptPackets;
    };
}

#endif // SCRIPT_DYNAMO_H