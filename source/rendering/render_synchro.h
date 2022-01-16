#ifndef RENDER_SYNCHRO_H
#define RENDER_SYNCHRO_H

//#include "intercession_pch.h"

#include "core/cosmos.h"
#include "render_dynamo.h"


namespace pleep
{
    // forward declare parent/owner
    class Cosmos;

    class RenderSynchro
    {
    public:
        RenderSynchro(Cosmos* owner, RenderDynamo* renderDynamo);
        ~RenderSynchro();

        // returns false if there was no attached dynamo to engage
        // THROWS runtime error if m_ownerCosmos is null
        void update(double deltaTime);

        // Synchros will also need a persisten state to change behaviour
        // they are a "part" of the cosmos so they have info about the cosmos
        // EX: global_hue_shift/global_scale/etc...
        
    private:
        // cosmos who created me and will proc my update
        // I am its friend and can access ECS
        Cosmos* m_ownerCosmos;

        // dynamo provided by cosmos context to engage on update
        RenderDynamo* m_attachedRenderDynamo;
    };
}

#endif // RENDER_SYNCHRO_H