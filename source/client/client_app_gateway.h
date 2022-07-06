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
        ~ClientAppGateway() override;
    };
}

#endif // CLIENT_APP_GATEWAY_H