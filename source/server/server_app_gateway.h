#ifndef SERVER_APP_GATEWAY_H
#define SERVER_APP_GATEWAY_H

//#include "intercession_pch.h"
#include <thread>
#include <vector>

#include "core/i_app_gateway.h"
#include "server/server_cosmos_context.h"
#include "networking/timeline_config.h"

namespace pleep
{
    class ServerAppGateway : public I_AppGateway
    {
    public:
        ServerAppGateway(TimelineConfig cfg);
        ~ServerAppGateway();

        void run() override;

    private:
        // Contexts representing each timeslice in this simulation
        std::vector<std::unique_ptr<I_CosmosContext>> m_contexts;

        // should I remember timelineApis/conduits for use after construction?
    };
}

#endif // SERVER_APP_GATEWAY_H