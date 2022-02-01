#include "render_synchro.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    RenderSynchro::RenderSynchro(Cosmos* owner) 
        : m_ownerCosmos(owner)
        , m_attachedRenderDynamo(nullptr)
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

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            UNREFERENCED_PARAMETER(entity);
            //TransformComponent transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            
            //m_attachedRenderDynamo->submit();
        }

        // once command queue is implemented this will flush them through relays
        m_attachedRenderDynamo->run_relays(deltaTime);

        // Do not flush Dynamo yet as other components (Context) may draw further
        // and then Context will flush at frame end
    }
    
    void RenderSynchro::attach_dynamo(RenderDynamo* contextDynamo) 
    {
        m_attachedRenderDynamo = contextDynamo;
    }
}