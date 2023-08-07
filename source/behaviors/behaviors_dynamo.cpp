#include "behaviors_dynamo.h"

namespace pleep
{
    BehaviorsDynamo::BehaviorsDynamo(std::shared_ptr<EventBroker> sharedBroker) 
        : A_Dynamo(sharedBroker)
    {
        PLEEPLOG_TRACE("Start behaviors pipeline setup");

        // any non-construction initialization
        
        PLEEPLOG_TRACE("Done behaviors pipeline setup");
    }
    
    BehaviorsDynamo::~BehaviorsDynamo() 
    {
    }
    
    void BehaviorsDynamo::submit(BehaviorsPacket data) 
    {
        // we'll store the packets ourselves
        m_behaviorsPackets.push_back(data);
    }
    
    void BehaviorsDynamo::run_relays(double deltaTime) 
    {
        for (std::vector<BehaviorsPacket>::iterator packet_it = m_behaviorsPackets.begin(); packet_it != m_behaviorsPackets.end(); packet_it++)
        {
            BehaviorsPacket& data = *packet_it;

            // TODO: Dynamos will need to seperate fixed/frame
            if (data.behaviors.use_fixed_update)
            {
                data.behaviors.drivetrain->on_fixed_update(deltaTime, data.behaviors, data.entity, data.owner);
            }
            //data.behaviors.drivetrain->on_frame_update(deltaTime, data.behaviors, data.entity, data.owner);
        }
    }
    
    void BehaviorsDynamo::reset_relays() 
    {
        m_behaviorsPackets.clear();
    }
}