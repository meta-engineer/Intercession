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

        // universal pre-frame init for dynamos
        virtual void prime();
        virtual void run_relays(double deltaTime) = 0;
    };
}

#endif // I_DYNAMO_H