#ifndef PHYSICS_CONTROL_SYNCHRO_H
#define PHYSICS_CONTROL_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "controlling/control_dynamo.h"

namespace pleep
{
    class PhysicsControlSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        // synchro needs a ControlDynamo to operate on
        void attach_dynamo(ControlDynamo* contextDynamo);
        
        // synchro can suggest to registry what signature to use from known cosmos
        // returns empty bitset if desired components could not be found
        static Signature get_signature(Cosmos* cosmos);

    private:
        // dynamo provided by CosmosContext to invoke on update
        ControlDynamo* m_attachedControlDynamo = nullptr;
    };
}

#endif // PHYSICS_CONTROL_SYNCHRO_H