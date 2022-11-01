#ifndef CLIENT_APP_GATEWAY_H
#define CLIENT_APP_GATEWAY_H

//#include "intercession_pch.h"
#include "core/i_app_gateway.h"
#include "client/client_cosmos_context.h"

namespace pleep
{
    class ClientAppGateway : public I_AppGateway
    {
    public:
        ClientAppGateway();
        ~ClientAppGateway();

        void run() override;

    private:
        // only 1 context (1 can be displayed at a time anyway)
        std::unique_ptr<I_CosmosContext> m_context = nullptr;
        // any need for the client context to run on a seperate thread?
    };
}

#endif // CLIENT_APP_GATEWAY_H