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

        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            PhysicsControlComponent& controller = m_ownerCosmos->get_component<PhysicsControlComponent>(entity);
            PhysicsComponent& physics = m_ownerCosmos->get_component<PhysicsComponent>(entity);
            
            m_attachedControlDynamo->submit(PhysicsControlPacket{ controller, transform, physics, entity });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    void PhysicsControlSynchro::attach_dynamo(ControlDynamo* contextDynamo) 
    {
        m_attachedControlDynamo = contextDynamo;
    }
    
    Signature PhysicsControlSynchro::get_signature(Cosmos* cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<TransformComponent>());
            sign.set(cosmos->get_component_type<PhysicsComponent>());
            sign.set(cosmos->get_component_type<PhysicsControlComponent>());
        }
        catch(const std::exception& e)
        {
            sign.reset();
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have TransformComponent, PhysicsComponent, and PhysicsControlComponent been registered?");
        }
        
        return sign;
    }
}