#ifndef SERVER_COSMOS_CONTEXT_H
#define SERVER_COSMOS_CONTEXT_H

//#include "intercession_pch.h"
#include "core/i_cosmos_context.h"

#include "networking/timeline_api.h"
#include "physics/physics_dynamo.h"
#include "server/server_network_dynamo.h"
#include "scripting/script_dynamo.h"

namespace pleep
{
    class ServerCosmosContext : public I_CosmosContext
    {
    public:
        // Accept all apis to use for lifetime,
        // (apis provide shared system resource for dynamos)
        // on construction we will build all the dynamos from these apis
        // localTimelineApi provides config to build network api
        //   api can be accessed through NetworkDynamo (or events it responds to)
        ServerCosmosContext(TimelineApi localTimelineApi);
        ~ServerCosmosContext();

    protected:
        void _prime_frame() override;
        void _on_fixed(double fixedTime) override;
        void _on_frame(double deltaTime) override;
        void _clean_frame() override;

        // populate the cosmos with synchros, and entities
        // provide registered synchros with our related dynamos
        // for now this has no parameters (scene filename in future?)
        void _build_cosmos();

        // Server specific Dynamos
        // Dynamos possess relevant api references passed in on construction from AppGateway
        // Our cosmos shares these dynamos with their synchros
        PhysicsDynamo* m_physicsDynamo;
        ServerNetworkDynamo* m_networkDynamo;
        ScriptDynamo* m_scriptDynamo;
    };
}

#endif // SERVER_COSMOS_CONTEXT_H