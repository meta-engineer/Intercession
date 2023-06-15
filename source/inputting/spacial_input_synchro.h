#ifndef SPACIAL_INPUT_SYNCHRO_H
#define SPACIAL_INPUT_SYNCHRO_H

//#include "intercession_pch.h"
#include <memory>

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

        Signature derive_signature() override;

        // synchro needs a InputDynamo to operate on
        void attach_dynamo(std::shared_ptr<InputDynamo> contextDynamo);

    private:
        // dynamo provided by CosmosContext to invoke on update
        std::shared_ptr<InputDynamo> m_attachedInputDynamo = nullptr;
    };
}

#endif // SPACIAL_INPUT_SYNCHRO_H