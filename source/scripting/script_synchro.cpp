#include "script_synchro.h"

#include "logging/pleep_log.h"
#include "core/cosmos.h"
#include "scripting/script_packet.h"

namespace pleep
{
    void ScriptSynchro::update() 
    {
        // No owner is a fatal error
        if (m_ownerCosmos == nullptr)
        {
            PLEEPLOG_ERROR("Synchro has no owner Cosmos");
            throw std::runtime_error("Script Synchro started update without owner Cosmos");
        }

        // no dynamo is a mistake, not necessarily an error
        if (m_attachedScriptDynamo == nullptr)
        {
            PLEEPLOG_WARN("Synchro update was called without an attached Dynamo");
            return;
        }
        
        for (Entity const& entity : m_entities)
        {
            ScriptComponent& script = m_ownerCosmos->get_component<ScriptComponent>(entity);
            
            m_attachedScriptDynamo->submit(ScriptPacket{ script, entity, m_ownerCosmos });
        }

        // Cosmos Context will flush dynamo relays once all synchros are done
    }
    
    void ScriptSynchro::attach_dynamo(ScriptDynamo* contextDynamo) 
    {
        m_attachedScriptDynamo = contextDynamo;
    }
    
    Signature ScriptSynchro::get_signature(Cosmos* cosmos) 
    {
        Signature sign;

        try
        {
            sign.set(cosmos->get_component_type<ScriptComponent>());
        }
        catch(const std::exception& e)
        {
            sign.reset();
            // Component Registry already logs error
            UNREFERENCED_PARAMETER(e);
            PLEEPLOG_ERROR("Synchro could not get desired component types from cosmos. Has ScriptComponent been registered?");
        }
        
        return sign;
    }
}