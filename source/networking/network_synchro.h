#ifndef NETWORK_SYNCHRO_H
#define NETWORK_SYNCHRO_H

//#include "intercession_pch.h"
#include "ecs/i_synchro.h"
#include "networking/i_network_dynamo.h"

namespace pleep
{
    class NetworkSynchro : public I_Synchro
    {
    public:
        // explicitly inherit constructors
        using I_Synchro::I_Synchro;

        // Provide entities of my sign, and registered ones to dynamo
        // exits early if there was no attached dynamo to use
        // THROWS runtime error if m_ownerCosmos is null
        void update() override;

        // synchro needs a RenderDynamo to operate on
        void attach_dynamo(I_NetworkDynamo* contextDynamo);

        // synchro can suggest to registry what signature to use from known cosmos
        // returns empty bitset if desired components could not be found
        static Signature get_signature(Cosmos* cosmos);

    private:
        // dynamo provided by cosmos context to invoke on update
        I_NetworkDynamo* m_attachedNetworkDynamo;
    };
}

#endif // NETWORK_SYNCHRO_H