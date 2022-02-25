#ifndef CONTROL_SYNCHRO_H
#define CONTROL_SYNCHRO_H

//#include "intercession_pch.h"

#include "ecs/i_synchro.h"
#include "controlling/control_dynamo.h"


namespace pleep
{
    class ControlSynchro : public ISynchro
    {
    public:
        // explicitly inherit constructors
        using ISynchro::ISynchro;
        
        // exits early if there was no attached dynamo to engage
        // THROWS runtime error if m_ownerCosmos is null
        void update(double deltaTime) override;

        // synchro needs a ControlDynamo to operate on
        void attach_dynamo(ControlDynamo* contextDynamo);

    private:
        // dynamo provided by CosmosContext to invoke on update
        ControlDynamo* m_attachedControlDynamo = nullptr;
    };
}

#endif // CONTROL_SYNCHRO_H