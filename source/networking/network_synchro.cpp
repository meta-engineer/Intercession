#include "network_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos_access_packet.h"

namespace pleep
{
    void NetworkSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Network Synchro started update without owner Cosmos");
        }
        
        // no dynamo is a mistake, not necessarily an error
        if (m_attachedNetworkDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }

        // TODO: remove this
        assert(m_entities.empty());

        // send only 1 packet
        m_attachedNetworkDynamo->submit(CosmosAccessPacket{ m_ownerCosmos });
    }
    
    Signature NetworkSynchro::derive_signature(Cosmos* cosmos) 
    {
        UNREFERENCED_PARAMETER(cosmos);
        // blank sign should mean NO matched entities
        return Signature();
    }
    
    void NetworkSynchro::attach_dynamo(I_NetworkDynamo* contextDynamo) 
    {
        // clear events registered through old dynamo

        m_attachedNetworkDynamo = contextDynamo;

        // restore event handlers
    }
}