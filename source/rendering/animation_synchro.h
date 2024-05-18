#ifndef ANIMATION_SYNCHRO_H
#define ANIMATION_SYNCHRO_H

//#include "intercession_pch.h"

#include "ecs/i_synchro.h"
#include "rendering/render_dynamo.h"

namespace pleep
{
    class AnimationSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;
        ~AnimationSynchro() override;

        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature() override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(std::shared_ptr<RenderDynamo> contextDynamo);

    private:
        // event handlers


        // dynamo provided by cosmos context to invoke on update
        std::shared_ptr<RenderDynamo> m_attachedRenderDynamo = nullptr;


        // Animation specific data

    };
}

#endif // ANIMATION_SYNCHRO_H