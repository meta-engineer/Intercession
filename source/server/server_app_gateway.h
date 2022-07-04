#ifndef SERVER_APP_GATEWAY_H
#define SERVER_APP_GATEWAY_H

//#include "intercession_pch.h"
#include "core/app_gateway.h"
#include "server/server_cosmos_context.h"

namespace pleep
{
    class ServerAppGateway : public AppGateway
    {
    protected:
        void _build_gateway() override;
        void _clean_gateway() override;
    };
}

#endif // SERVER_APP_GATEWAY_H