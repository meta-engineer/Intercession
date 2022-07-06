#ifndef SERVER_APP_GATEWAY_H
#define SERVER_APP_GATEWAY_H

//#include "intercession_pch.h"
#include "core/i_app_gateway.h"
#include "server/server_cosmos_context.h"

namespace pleep
{
    class ServerAppGateway : public I_AppGateway
    {
    public:
        ServerAppGateway();
        ~ServerAppGateway() override;
    };
}

#endif // SERVER_APP_GATEWAY_H