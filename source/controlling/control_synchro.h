#ifndef CONTROL_SYNCHRO_H
#define CONTROL_SYNCHRO_H

//#include "intercession_pch.h"

#include "core/cosmos.h"
#include "control_dynamo.h"


namespace pleep
{
    // forward declare parent/owner
    class Cosmos;

    class ControlSynchro
    {
    public:
        ControlSynchro(Cosmos* owner, ControlDynamo* controlDynamo);
        ~ControlSynchro();
        

        void update(double deltaTime);

        // Synchros will also need a persistent state to change behaviour
        // they are a "part" of the cosmos so they have info about the cosmos
        // EX: disabled/enabled/mouse_only/typing/etc...

    private:
        // cosmos who created me and will proc my update
        // I am its friend and can access ECS
        Cosmos* m_ownerCosmos;

        // dynamo provided by cosmos context to engage on update
        ControlDynamo* m_attachedControlDynamo;
    };
}

#endif // CONTROL_SYNCHRO_H