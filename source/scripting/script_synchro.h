#ifndef SCRIPT_SYNCHRO_H
#define SCRIPT_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "scripting/script_dynamo.h"

namespace pleep
{
    // Basic Synchro behaviour for ScriptComponent
    class ScriptSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature(std::shared_ptr<Cosmos> cosmos) override;

        // synchro needs a ScriptDynamo to operate on
        void attach_dynamo(ScriptDynamo* contextDynamo);

    private:
        // dynamo provided by CosmosContext to invoke on update
        ScriptDynamo* m_attachedScriptDynamo = nullptr;
    };
}

#endif // SCRIPT_SYNCHRO_H