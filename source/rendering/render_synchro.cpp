#include "render_synchro.h"

#include <exception>
#include "logging/pleep_log.h"
#include "core/cosmos.h"

namespace pleep
{
    RenderSynchro::RenderSynchro(Cosmos* owner, RenderDynamo* renderDynamo) 
        : m_ownerCosmos(owner)
        , m_attachedRenderDynamo(renderDynamo)
    {
        
    }
    
    RenderSynchro::~RenderSynchro() 
    {
        // I do not own my Cosmos (parent) or Dynamo
    }
    
    void RenderSynchro::update(double deltaTime)
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Render Synchro does not owner Cosmos");
            throw std::runtime_error("Render Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedRenderDynamo == nullptr)
        {
            PLEEPLOG_WARN("Render Synchro update was called without an attached Dynamo");
            return;
        }
        

        // any dynamo specific initialization (glClear)
        m_attachedRenderDynamo->prime();

        // use friendly owner Cosmos' ECS to feed Render Components to attached RenderDynamo
        //m_attachedRenderDynamo->submit();

        // once command queue is implemented this will flush them through relays
        m_attachedRenderDynamo->run_relays(deltaTime);

        // Do not yet flush Dynamo as other components (Context) may draw further
    }
}