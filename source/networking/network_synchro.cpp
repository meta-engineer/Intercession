#include "network_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos_access_packet.h"

namespace pleep
{
    void NetworkSynchro::update() 
    {
        std::shared_ptr<Cosmos> cosmos = m_ownerCosmos.expired() ? nullptr : m_ownerCosmos.lock();
        // No owner is a fatal error
        if (cosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("NetworkSynchro started update without owner Cosmos");
        }
        
        // no dynamo is a mistake, not necessarily an error
        if (m_attachedNetworkDynamo == nullptr)
        {
            //PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        // TODO: remove this
        assert(m_entities.empty());

        // send only 1 packet
        m_attachedNetworkDynamo->submit(CosmosAccessPacket{ m_ownerCosmos });
    }
    
    Signature NetworkSynchro::derive_signature() 
    {
        // blank sign should mean NO matched entities
        return Signature();
    }
    
    void NetworkSynchro::attach_dynamo(std::shared_ptr<I_NetworkDynamo> contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedNetworkDynamo = contextDynamo;

        // restore event handlers
    }
}