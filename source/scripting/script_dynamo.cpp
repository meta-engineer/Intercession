#include "script_dynamo.h"

namespace pleep
{
    ScriptDynamo::ScriptDynamo(EventBroker* sharedBroker) 
        : A_Dynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Start script pipeline setup");

        // any non-construction initialization
        
        PLEEPLOG_TRACE("Done script pipeline setup");
    }
    
    ScriptDynamo::~ScriptDynamo() 
    {
    }
    
    void ScriptDynamo::submit(ScriptPacket data) 
    {
        // we'll store the packets ourselves
        m_scriptPackets.push_back(data);
    }
    
    void ScriptDynamo::run_relays(double deltaTime) 
    {
        for (std::vector<ScriptPacket>::iterator packet_it = m_scriptPackets.begin(); packet_it != m_scriptPackets.end(); packet_it++)
        {
            ScriptPacket& data = *packet_it;

            // TODO: Dynamos will need to seperate fixed/frame
            if (data.script.use_fixed_update)
            {
                data.script.drivetrain->on_fixed_update(deltaTime, data.script, data.entity, data.owner);
            }
            //data.script.drivetrain->on_frame_update(deltaTime, data.entity, data.owner);
        }
    }
    
    void ScriptDynamo::reset_relays() 
    {
        m_scriptPackets.clear();
    }
}