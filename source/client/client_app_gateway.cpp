#include "client_app_gateway.h"

#include "logging/pleep_log.h"

namespace pleep
{
    ClientAppGateway::ClientAppGateway() 
    {
        PLEEPLOG_TRACE("Configuring Client App Gateway");
        // build apis for my specific context
        this->_build_window_api();
        
        // inline _build_context
        // be strict and LOUD with misuse?
        assert(!m_context);
        m_context = std::make_unique<ClientCosmosContext>(m_windowApi);
    }
    
    ClientAppGateway::~ClientAppGateway() 
    {
        // inline _clean_context
        assert(m_context);
        m_context->stop();

        // If there is only 1 thread in client destructor should only be called when main deletes
        // do we have to wait for context to "successfully" stop before cleaning apis?

        // cleanup my context specific apis
        this->_clean_window_api();
    }

    void ClientAppGateway::run()
    {
        if (!m_context)
        {
            PLEEPLOG_ERROR("AppGateway cannot be run with null context");
            return;
        }

        PLEEPLOG_TRACE("App run begin");
        std::ostringstream thisThreadId; thisThreadId << std::this_thread::get_id();
        PLEEPLOG_INFO("AppGateway thread #" + thisThreadId.str());

        while(true)
        {
            m_context->run();
            
            // TODO: handle context closing? check context for some state for next context?
            // does it make sense for context to change? 
            // does it make sense for cosmos to change, or just to be cleared and re-populated?
            // It is more difficult for server to handle regular context closing (because they're async)
            // so it may be preferable to have context closing ONLY be in fatal cases

            // TEMP: always handle context stopping by also stopping ourselves
            break;
        }

        PLEEPLOG_TRACE("App run end");
    }
}