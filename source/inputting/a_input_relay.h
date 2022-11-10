#ifndef A_INPUT_RELAY_H
#define A_INPUT_RELAY_H

//#include "intercession_pch.h"
#include "inputting/raw_input_buffer.h"

namespace pleep
{
    class A_InputRelay
    {
    protected:
        A_InputRelay(const RawInputBuffer& sharedBuffer)
            : m_sharedBuffer(sharedBuffer)
        {}
    public:
        virtual ~A_InputRelay() = default;

        // subclasses should have submit methods for their specific packet type

        // submittions are done, shared buffer is set
        virtual void engage(double deltaTime) = 0;

        // clear any leftover submittions
        virtual void clear() = 0;
    protected:
        // refrence to resource owned by dynamo, shared with other relays
        // (this is like assigning framebuffer resources for rendering)
        const RawInputBuffer& m_sharedBuffer;        
    };
}

#endif // A_INPUT_RELAY_H