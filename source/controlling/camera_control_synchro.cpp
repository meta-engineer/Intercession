#include "camera_control_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "controlling/camera_control_packet.h"

namespace pleep
{
    void CameraControlSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Camera Control Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedControlDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            CameraControlComponent& controller = m_ownerCosmos->get_component<CameraControlComponent>(entity);
            
            m_attachedControlDynamo->submit(CameraControlPacket{ controller, transform, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    void CameraControlSynchro::attach_dynamo(ControlDynamo* contextDynamo) 
    {
        m_attachedControlDynamo = contextDynamo;
    }
}