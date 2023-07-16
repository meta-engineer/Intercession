#ifndef BEHAVIORS_SYNCHRO_H
#define BEHAVIORS_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "behaviors/behaviors_dynamo.h"

namespace pleep
{
    // Basic Synchro behaviour for BehaviorsComponent
    class BehaviorsSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature() override;

        // synchro needs a BehaviorsDynamo to operate on
        void attach_dynamo(std::shared_ptr<BehaviorsDynamo> contextDynamo);

    private:
        // dynamo provided by CosmosContext to invoke on update
        std::shared_ptr<BehaviorsDynamo> m_attachedBehaviorsDynamo = nullptr;
    };
}

#endif // BEHAVIORS_SYNCHRO_H