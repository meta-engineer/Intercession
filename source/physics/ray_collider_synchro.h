#ifndef RAY_COLLIDER_SYNCHRO_H
#define RAY_COLLIDER_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "physics/physics_dynamo.h"

namespace pleep
{
    class RayColliderSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature(Cosmos* cosmos) override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(PhysicsDynamo* contextDynamo);

    private:
        // dynamo provided by cosmos context to invoke on update
        PhysicsDynamo* m_attachedPhysicsDynamo;
    };
}

#endif // RAY_COLLIDER_SYNCHRO_H