#include "physics_control_synchro.h"

#include "logging/pleep_log.h"
#include "controlling/camera_control_packet.h"

namespace pleep
{
    void PhysicsControlSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Physics Control Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedControlDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        // We'll enfoce that entities with camera controllers also have camera (render) components
        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            PhysicsControlComponent& controller = m_ownerCosmos->get_component<PhysicsControlComponent>(entity);
            PhysicsComponent& physics = m_ownerCosmos->get_component<PhysicsComponent>(entity);
            
            m_attachedControlDynamo->submit(PhysicsControlPacket{ controller, transform, physics });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    void PhysicsControlSynchro::attach_dynamo(ControlDynamo* contextDynamo) 
    {
        m_attachedControlDynamo = contextDynamo;
    }
}