#ifndef CLIENT_COSMOS_CONTEXT_H
#define CLIENT_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include <memory>
#include "core/i_cosmos_context.h"

#include "rendering/render_dynamo.h"
#include "inputting/input_dynamo.h"
#include "physics/physics_dynamo.h"
#include "client/client_network_dynamo.h"

namespace pleep
{
    class ClientCosmosContext : public I_CosmosContext
    {
    public:
        // Accept all apis to use for lifetime,
        // (apis provide shared system resource for dynamos)
        // on construction we will build all the dynamos from these apis
        ClientCosmosContext(GLFWwindow* windowApi);
        ~ClientCosmosContext();

    protected:
        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;
        
        // populate the cosmos
        // for now this has no parameters (scene filename in future?)
        // provide registered synchros with our dynamos
        void _build_cosmos();

        // Client specific Dynamos
        // keep concrete, non-interface references incase we need to retrieve specific data
        // Dynamos possess relevant api references passed in on construction from AppGateway
        // Our cosmos shares these dynamos with their synchros
        RenderDynamo* m_renderDynamo;
        InputDynamo* m_inputDynamo;
        PhysicsDynamo* m_physicsDynamo;
        ClientNetworkDynamo* m_networkDynamo;
        //AudioDynamo* m_audioDynamo;
    };
}

#endif // CLIENT_COSMOS_CONTEXT_H