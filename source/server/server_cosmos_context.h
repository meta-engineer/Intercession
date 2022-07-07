#ifndef SERVER_COSMOS_CONTEXT_H
#define SERVER_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include "core/i_cosmos_context.h"

#include "controlling/control_dynamo.h"
#include "physics/physics_dynamo.h"
#include "server/server_network_dynamo.h"

namespace pleep
{
    class ServerCosmosContext : public I_CosmosContext
    {
    public:
        // Accept all apis to use for lifetime,
        // (apis provide shared system resource for dynamos)
        // on construction we will build all the dynamos from these apis
        // TODO: separate asioContext into NetworkApi?
        // do multiple dynamos need access to network api? or should it be owned by NetDynamo?
        ServerCosmosContext();
        ~ServerCosmosContext();

    protected:
        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;

        // Server specific Dynamos
        // Dynamos possess relevant api references passed in on construction from AppGateway
        // Our cosmos shares these dynamos with their synchros
        //ControlDynamo* m_controlDynamo;
        PhysicsDynamo* m_physicsDynamo;
        ServerNetworkDynamo* m_networkDynamo;
        //AudioDynamo* m_audioDynamo;
    };
}

#endif // SERVER_COSMOS_CONTEXT_H