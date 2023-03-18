#include "physics_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/physics_packet.h"

namespace pleep
{
    void PhysicsSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Physics Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedPhysicsDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = m_ownerCosmos->get_component<TransformComponent>(entity);
            PhysicsComponent& physics = m_ownerCosmos->get_component<PhysicsComponent>(entity);
            
            m_attachedPhysicsDynamo->submit(PhysicsPacket{ transform, physics });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    Signature PhysicsSynchro::derive_signature(Cosmos* cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<TransformComponent>());
            sign.set(cosmos->get_component_type<PhysicsComponent>());
        }
        catch(const std::exception& e)
        {
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            sign.reset();
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have TransformComponent and PhysicsComponent been registered?");
        }
        
        return sign;
    }
    
    void PhysicsSynchro::attach_dynamo(PhysicsDynamo* contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedPhysicsDynamo = contextDynamo;

        // restore event handlers
    }
}