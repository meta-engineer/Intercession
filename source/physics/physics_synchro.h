#ifndef PHYSICS_SYNCHRO_H
#define PHYSICS_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "physics/physics_dynamo.h"


namespace pleep
{
    class PhysicsSynchro : public ISynchro
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

        // synchro can suggest to registry what signature to use from known cosmos
        // returns empty bitset if desired components could not be found
        static Signature get_signature(Cosmos* cosmos);
        
    private:
        // dynamo provided by cosmos context to invoke on update
        PhysicsDynamo* m_attachedPhysicsDynamo;
    };
}

#endif // PHYSICS_SYNCHRO_H