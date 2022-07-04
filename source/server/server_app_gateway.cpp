#include "server_app_gateway.h"

namespace pleep
{
    void ServerAppGateway::_build_gateway() 
    {
        PLEEPLOG_TRACE("Configuring Server App Gateway");
        // no apis needed for now...

        // inline _build_context
        // be strict and LOUD with misuse?
        assert(!m_ctx);
        m_ctx = new ServerCosmosContext();
    }
    
    void ServerAppGateway::_clean_gateway() 
    {
        // inline _clean_context
        assert(m_ctx);
        m_ctx->stop();
        // wait for thread to join?
        delete m_ctx;
        
        // no apis to clean for now...
    }

    
}