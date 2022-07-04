#ifndef CLIENT_APP_GATEWAY_H
#define CLIENT_APP_GATEWAY_H

//#include "intercession_pch.h"
#include "core/app_gateway.h"
#include "client/client_cosmos_context.h"

namespace pleep
{
    class ClientAppGateway : public AppGateway
    {    
    protected:
        void _build_gateway() override;
        void _clean_gateway() override;
    };
}

#endif // CLIENT_APP_GATEWAY_H