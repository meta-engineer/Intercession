#ifndef PHYSICS_CONTROL_SYNCHRO_H
#define PHYSICS_CONTROL_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "controlling/control_dynamo.h"

namespace pleep
{
    class PhysicsControlSynchro : public ISynchro
    {
    public:
        // explicitly inherit constructors
        using ISynchro::ISynchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        // synchro needs a ControlDynamo to operate on
        void attach_dynamo(ControlDynamo* contextDynamo);
        
    private:
        // dynamo provided by CosmosContext to invoke on update
        ControlDynamo* m_attachedControlDynamo = nullptr;
    };
}

#endif // PHYSICS_CONTROL_SYNCHRO_H