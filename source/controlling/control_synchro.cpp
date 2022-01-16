#include "control_synchro.h"

#include <exception>
#include "logging/pleep_log.h"
#include "core/cosmos.h"

namespace pleep
{
    ControlSynchro::ControlSynchro(Cosmos* owner, ControlDynamo* controlDynamo)
        : m_ownerCosmos(owner)
        , m_attachedControlDynamo(controlDynamo)
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
            m_ownerCosmos->m_status = Cosmos::Status::FAIL;
            return;
        }

        // use friendly owner Cosmos' ECS to feed control components to attached ControlDynamo

        // any dynamo specific initialization
        m_attachedControlDynamo->prime();

        m_attachedControlDynamo->run_relays(deltaTime);
        
        // handle final dynamo signals
        switch(m_attachedControlDynamo->get_signal())
        {
            case (IDynamo::Signal::FAIL):
                // dynamo will report error, what how should cosmos react? just end?
                break;
            case (IDynamo::Signal::CLOSE):
                m_ownerCosmos->m_status = Cosmos::Status::CLOSE;
                break;
            default:
                break;
        }
    }
}