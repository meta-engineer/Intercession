#ifndef BOX_COLLIDER_SYNCHRO_H
#define BOX_COLLIDER_SYNCHRO_H

//#include "intercession_pch.h"
#include <memory>

#include "ecs/i_synchro.h"
#include "physics/physics_dynamo.h"

namespace pleep
{
    class BoxColliderSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        Signature derive_signature() override;

        // synchro needs a PhysicsDynamo to operate on
        void attach_dynamo(std::shared_ptr<PhysicsDynamo> contextDynamo);
        
    private:
        // dynamo provided by cosmos context to invoke on update
        std::shared_ptr<PhysicsDynamo> m_attachedPhysicsDynamo;
    };
}

#endif // BOX_COLLIDER_SYNCHRO_H