#ifndef RENDER_SYNCHRO_H
#define RENDER_SYNCHRO_H

//#include "intercession_pch.h"

#include "core/cosmos.h"
#include "render_dynamo.h"


namespace pleep
{
    // forward declare parent/owner
    class Cosmos;

    class RenderSynchro : public ISynchro
    {
    public:
        // Start with empty entity set
        // my entity signature is managed by SynchroRegistry I should be a part of
        // Provided Cosmos which is using my related ECS registries (which i'll access)
        RenderSynchro(Cosmos* owner);
        ~RenderSynchro();

        // exits early if there was no attached dynamo to engage
        // THROWS runtime error if m_ownerCosmos is null
        void update(double deltaTime) override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(RenderDynamo* contextDynamo);
        
    private:
        // cosmos who created me and will proc my update
        // I am its friend and can access ECS
        Cosmos* m_ownerCosmos;

        // dynamo provided by cosmos context to invoke on update
        RenderDynamo* m_attachedRenderDynamo;
    };
}

#endif // RENDER_SYNCHRO_H