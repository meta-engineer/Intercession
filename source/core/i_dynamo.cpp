#include "i_dynamo.h"

namespace pleep
{
    IDynamo::IDynamo() 
    {
        
    }
    
    IDynamo::~IDynamo() 
    {
        
    }
    
    void IDynamo::prime() 
    {
        m_signal = IDynamo::Signal::OK;
    }
    
    IDynamo::Signal IDynamo::get_signal() const
    {
        return m_signal;
    }
    
    IDynamo::Signal IDynamo::get_and_clear_signal() 
    {
        IDynamo::Signal tmp = m_signal;
        m_signal = IDynamo::Signal::OK;
        return tmp;
    }
}