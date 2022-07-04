#include "client_app_gateway.h"

namespace pleep
{
    void ClientAppGateway::_build_gateway() 
    {
        PLEEPLOG_TRACE("Configuring Client App Gateway");
        // build apis for my specific context
        this->_build_window_api();
        
        // inline _build_context
        // be strict and LOUD with misuse?
        assert(!m_ctx);
        m_ctx = new ClientCosmosContext(m_windowApi);
    }
    
    void ClientAppGateway::_clean_gateway() 
    {
        // inline _clean_context
        assert(m_ctx);
        m_ctx->stop();
        // wait for thread to join?
        delete m_ctx;

        // cleanup my context specific apis
        this->_clean_window_api();
    }

}