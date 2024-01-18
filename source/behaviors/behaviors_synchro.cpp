#include "behaviors_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"
#include "behaviors/behaviors_packet.h"

namespace pleep
{
    void BehaviorsSynchro::update() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        // No owner is a fatal error
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("BehaviorsSynchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedBehaviorsDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        for (Entity const& entity : m_entities)
        {
            BehaviorsComponent& behaviors = cosmos->get_component<BehaviorsComponent>(entity);

            m_attachedBehaviorsDynamo->submit(BehaviorsPacket{ behaviors, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    Signature BehaviorsSynchro::derive_signature() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.lock();
        if (m_ownerCosmos.expired())
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("BehaviorsSynchro started signature derivation without owner Cosmos");
        }

        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<BehaviorsComponent>());
        }
        catch(const std::exception& e)
        {
            sign.reset();
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Has BehaviorsComponent been registered?");
        }
        
        return sign;
    }
    
    void BehaviorsSynchro::attach_dynamo(std::shared_ptr<BehaviorsDynamo> contextDynamo) 
    {
        m_attachedBehaviorsDynamo = contextDynamo;
    }
}