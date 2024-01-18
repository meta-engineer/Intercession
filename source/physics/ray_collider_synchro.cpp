#include "ray_collider_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/collider_packet.h"
#include "physics/ray_collider_component.h"

namespace pleep
{
    void RayColliderSynchro::update() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("RayColliderSynchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedPhysicsDynamo == nullptr)
        {
            //PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        for (Entity const& entity : m_entities)
        {
            TransformComponent& transform = cosmos->get_component<TransformComponent>(entity);
            RayColliderComponent& ray = cosmos->get_component<RayColliderComponent>(entity);

            // RESET ray max collider distance before sending each frame
            ray.reset();
            
            m_attachedPhysicsDynamo->submit(ColliderPacket{ transform, &ray, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    Signature RayColliderSynchro::derive_signature() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("RayColliderSynchro started signature derivation without owner Cosmos");
        }

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
    
    void RayColliderSynchro::attach_dynamo(std::shared_ptr<PhysicsDynamo> contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedPhysicsDynamo = contextDynamo;

        // restore event handlers
    }
}