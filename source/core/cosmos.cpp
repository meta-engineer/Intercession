#include "cosmos.h"

#include "logging/pleep_log.h"

namespace pleep
{
    Cosmos::Cosmos(RenderDynamo* renderDynamo, ControlDynamo* controlDynamo) 
    {
        // assume Dynamos are already configured and cross-configured as needed

        m_renderSynch  = new RenderSynchro(this, renderDynamo);

        m_controlSynch = new ControlSynchro(this, controlDynamo);
    }

    Cosmos::~Cosmos() 
    {
        delete m_renderSynch;
        delete m_controlSynch;
    }
    
    void Cosmos::update(double deltaTime) 
    {
        // synchros will feed Entities into dynamos, results will mutate Components
        m_controlSynch->update(deltaTime);
        m_renderSynch->update(deltaTime);
    }

}