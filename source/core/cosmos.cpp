#include "cosmos.h"

#include "logging/pleep_log.h"

namespace pleep
{
    Cosmos::Cosmos() 
    {
        
    }

    Cosmos::~Cosmos() 
    {
        
    }
    
    bool Cosmos::update(double deltaTime) 
    {
        UNREFERENCED_PARAMETER(deltaTime);
        
        //m_renderSynch->update();

        // Cosmos has finished, and no longer wishes to be called
        return false;
    }
}