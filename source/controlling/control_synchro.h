#ifndef CONTROL_SYNCHRO_H
#define CONTROL_SYNCHRO_H

//#include "intercession_pch.h"

#include "ecs/i_synchro.h"
#include "core/cosmos.h"
#include "control_dynamo.h"


namespace pleep
{
    // forward declare parent/owner
    class Cosmos;

    class ControlSynchro : public ISynchro
    {
    public:
        // Start with empty entity set
        // my entity signature is managed by SynchroRegistry I should be a part of
        // Provided Cosmos which is using my related ECS registries (which i'll access)
        ControlSynchro(Cosmos* owner);
        ~ControlSynchro();
        
        // exits early if there was no attached dynamo to engage
        // THROWS runtime error if m_ownerCosmos is null
        void update(double deltaTime) override;

        // synchro needs a ControlDynamo to operate on
        void attach_dynamo(ControlDynamo* contextDynamo);

    private:
        // cosmos who created me and will proc my update
        // I am its friend and can access ECS
        Cosmos* m_ownerCosmos;

        // dynamo provided by CosmosContext to invoke on update
        ControlDynamo* m_attachedControlDynamo;
    };
}

#endif // CONTROL_SYNCHRO_H