#ifndef LIGHTING_SYNCHRO_H
#define LIGHTING_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "rendering/render_dynamo.h"

namespace pleep
{
    class LightingSynchro : public ISynchro
    {
    public:
        // explicitly inherit constructors
        using ISynchro::ISynchro;

        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update(double deltaTime) override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(RenderDynamo* contextDynamo);

    private:
        
        // dynamo provided by cosmos context to invoke on update
        RenderDynamo* m_attachedRenderDynamo = nullptr;
    };
}

#endif // LIGHTING_SYNCHRO_H