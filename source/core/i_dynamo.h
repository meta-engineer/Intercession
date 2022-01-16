#ifndef I_DYNAMO_H
#define I_DYNAMO_H

//#include "intercession_pch.h"

namespace pleep
{
    class IDynamo
    {
    public:
        IDynamo();
        ~IDynamo();

        // indicate overall result of user calls since last prime()
        // Synchros (callers) or Context (owner) can poll this result for runtime status
        // this value only has "immediate" relevance for this frame
        //   (unlike a Cosmos' status which is a persistent state)
        enum class Signal {
            OK,
            CLOSE,
            FAIL,
        };

        // universal pre-frame init for dynamos
        virtual void prime();
        virtual void run_relays(double deltaTime) = 0;

        // get signal flag value
        IDynamo::Signal get_signal() const;
        // get signal flag value and then reset to OK
        IDynamo::Signal get_and_clear_signal();

    protected:
        // after Dynamo powers relays, failures/status/result is stored here
        IDynamo::Signal m_signal = IDynamo::Signal::OK;
    };
}

#endif // I_DYNAMO_H