#include "spacial_input_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"
#include "inputting/spacial_input_packet.h"

namespace pleep
{
    void SpacialInputSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Spacial Input Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedInputDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }
        
        for (Entity const& entity : m_entities)
        {
            SpacialInputComponent& input = m_ownerCosmos->get_component<SpacialInputComponent>(entity);
            
            m_attachedInputDynamo->submit(SpacialInputPacket{ input });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    Signature SpacialInputSynchro::derive_signature(Cosmos* cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<SpacialInputComponent>());
        }
        catch(const std::exception& e)
        {
            sign.reset();
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Has SpacialInputComponent been registered?");
        }
        
        return sign;
    }
    
    void SpacialInputSynchro::attach_dynamo(InputDynamo* contextDynamo) 
    {
        m_attachedInputDynamo = contextDynamo;
    }
}