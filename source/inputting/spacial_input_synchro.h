#ifndef SPACIAL_INPUT_SYNCHRO_H
#define SPACIAL_INPUT_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "inputting/input_dynamo.h"

namespace pleep
{
    class SpacialInputSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature(Cosmos* cosmos) override;

        // synchro needs a ControlDynamo to operate on
        void attach_dynamo(InputDynamo* contextDynamo);

    private:
        // dynamo provided by CosmosContext to invoke on update
        InputDynamo* m_attachedInputDynamo = nullptr;
    };
}

#endif // SPACIAL_INPUT_SYNCHRO_H