#include "spacetime_synchro.h"

#include "logging/pleep_log.h"
#include "spacetime/spacetime_packet.h"

namespace pleep
{
    void SpacetimeSynchro::update() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.expired() ? nullptr : m_ownerCosmos.lock();
        // No owner is a fatal error
        if (cosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("SpacetimeSynchro started update without owner Cosmos");
        }
        
        // no dynamo is a mistake, not necessarily an error
        if (m_attachedNetworkDynamo == nullptr)
        {
            //PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        for (Entity const& entity : m_entities)
        {
            SpacetimeComponent& spacetime = cosmos->get_component<SpacetimeComponent>(entity);

            // cleanup merged components?

            m_attachedNetworkDynamo->submit(SpacetimePacket{ entity, spacetime });
        }
    }

    Signature SpacetimeSynchro::derive_signature() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.expired() ? nullptr : m_ownerCosmos.lock();
        if (cosmos == nullptr)
        {
            PLEEPLOG_ERROR("Cannot derive signature for null Cosmos");
            throw std::runtime_error("SpacetimeSynchro started signature derivation without owner Cosmos");
        }

        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<SpacetimeComponent>());
        }
        catch(const std::exception& e)
        {
            sign.reset();
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Has SpacetimeComponent been registered?");
        }
        
        return sign;
    }

    void SpacetimeSynchro::attach_dynamo(std::shared_ptr<I_NetworkDynamo> contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedNetworkDynamo = contextDynamo;

        // restore event handlers
    }

    
} // namespace pleep
