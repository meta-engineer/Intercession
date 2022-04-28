#ifndef BOX_COLLIDER_SYNCHRO_H
#define BOX_COLLIDER_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "physics/physics_dynamo.h"

namespace pleep
{
    class BoxColliderSynchro : public ISynchro
    {
    public:
        // explicitly inherit constructors
        using ISynchro::ISynchro;

        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(PhysicsDynamo* contextDynamo);

    private:
        // dynamo provided by cosmos context to invoke on update
        PhysicsDynamo* m_attachedPhysicsDynamo;
    };
}

#endif // BOX_COLLIDER_SYNCHRO_H