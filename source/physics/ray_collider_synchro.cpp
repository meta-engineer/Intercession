#include "ray_collider_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/collider_packet.h"
#include "physics/ray_collider_component.h"

namespace pleep
{
    void RayColliderSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Box Collider Synchro started update without owner Cosmos");
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
            RayColliderComponent& ray = m_ownerCosmos->get_component<RayColliderComponent>(entity);
            
            m_attachedPhysicsDynamo->submit(ColliderPacket{ transform, &ray, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    void RayColliderSynchro::attach_dynamo(PhysicsDynamo* contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedPhysicsDynamo = contextDynamo;

        // restore event handlers
    }
    
    Signature RayColliderSynchro::get_signature(Cosmos* cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<TransformComponent>());
            sign.set(cosmos->get_component_type<RayColliderComponent>());
        }
        catch(const std::exception& e)
        {
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            sign.reset();
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have TransformComponent and RayColliderComponent been registered?");
        }
        
        return sign;
    }
}