#include "box_collider_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"

#include "physics/collider_packet.h"
#include "physics/box_collider_component.h"

namespace pleep
{
    void BoxColliderSynchro::update() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("BoxColliderSynchro started update without owner Cosmos");
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
            BoxColliderComponent& box = cosmos->get_component<BoxColliderComponent>(entity);
            
            m_attachedPhysicsDynamo->submit(ColliderPacket{ transform, &box, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    Signature BoxColliderSynchro::derive_signature() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.expired() ? nullptr : m_ownerCosmos.lock();
        if (cosmos == nullptr)
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("BoxColliderSynchro started signature derivation without owner Cosmos");
        }

        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<TransformComponent>());
            sign.set(cosmos->get_component_type<BoxColliderComponent>());
        }
        catch(const std::exception& e)
        {
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            sign.reset();
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Have TransformComponent and BoxColliderComponent been registered?");
        }
        
        return sign;
    }
    
    void BoxColliderSynchro::attach_dynamo(std::shared_ptr<PhysicsDynamo> contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedPhysicsDynamo = contextDynamo;

        // restore event handlers
    }
}