#ifndef COSMOS_H
#define COSMOS_H

//#include "intercession_pch.h"

namespace pleep
{
    // We could have a Cosmos interface
    // a subclass could have a character controller, ecs, graphics/phsyics
    // a subclass could have only mouse and ui
    class Cosmos
    {
    public:
        Cosmos();
        ~Cosmos();

        //void update(double deltaTime);

    private:
        // use ECS (Entity, Component, Synchro) pattern to optimize update calls
        //EntityRegistry* m_registry;

        // ECS synchros know their respective dynamos and feed components into them on update
        // synchros are created with their required dynamo (would dynamically attaching be useful?)
        // deleting or mutating a dynamo must apply to any synchros attached to it
        // to avoid dereferencing an invalid dynamo (done by CosmosManager?)
        
        //RenderSynchro* m_renderSynch;

        //InputSynchro*
        //PhysicsSynchro*
        //AudioSynchro*
        //NetSynchro*
        // etc...
    };
}

#endif // COSMOS_H