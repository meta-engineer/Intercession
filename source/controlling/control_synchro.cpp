#include "control_synchro.h"

#include <exception>
#include "logging/pleep_log.h"

namespace pleep
{
    ControlSynchro::ControlSynchro(Cosmos* owner)
        : m_ownerCosmos(owner)
        , m_attachedControlDynamo(nullptr)
    {
        
    }
    
    ControlSynchro::~ControlSynchro() 
    {
        // I do not own my Cosmos (parent) or Dynamo
    }
    
    void ControlSynchro::update(double deltaTime) 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Control Synchro does not owner Cosmos");
            throw std::runtime_error("Control Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedControlDynamo == nullptr)
        {
            PLEEPLOG_WARN("Control Synchro update was called without an attached Dynamo");
            return;
        }
        
        // any dynamo specific initialization
        m_attachedControlDynamo->prime();

        // feed components of m_entities to attached ControlDynamo
        // I should implicitly know my signature and therefore what components i can fetch
        for (Entity const& entity : m_entities)
        {
            UNREFERENCED_PARAMETER(entity);
            //m_ownerCosmos->get_component<Transform>(entity);
        }

        m_attachedControlDynamo->run_relays(deltaTime);
    }
    
    void ControlSynchro::attach_dynamo(ControlDynamo* contextDynamo) 
    {
        m_attachedControlDynamo = contextDynamo;
    }
}