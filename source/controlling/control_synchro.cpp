#include "control_synchro.h"

#include <exception>
#include "logging/pleep_log.h"
#include "core/cosmos.h"
#include "controlling/control_component.h"
#include "controlling/control_packet.h"

namespace pleep
{   
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

        // more than 1 entity could be controlled at a time
        // eg. mouse controls camera, keys control character
        // so we should process all controllable entities...
        // ControlComponents should have an "active" state to determine if they should receive input
        for (Entity const& entity : m_entities)
        {
            ControlComponent& controller = m_ownerCosmos->get_component<ControlComponent>(entity);
            
            // if I pass a packet of component references, 
            // will passing the packet by value maintain the references?
            m_attachedControlDynamo->submit(ControlPacket{ controller, entity, m_ownerCosmos });
        }

        m_attachedControlDynamo->run_relays(deltaTime);
    }
    
    void ControlSynchro::attach_dynamo(ControlDynamo* contextDynamo) 
    {
        m_attachedControlDynamo = contextDynamo;
    }
}