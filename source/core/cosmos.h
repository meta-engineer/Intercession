#ifndef COSMOS_H
#define COSMOS_H

//#include "intercession_pch.h"
#include "rendering/render_dynamo.h"
#include "rendering/render_synchro.h"
#include "controlling/control_dynamo.h"
#include "controlling/control_synchro.h"

namespace pleep
{
    // We could have a Cosmos interface
    // a subclass could have a character controller, ecs, graphics/phsyics
    // a subclass could have only mouse and ui
    class Cosmos
    {
    public:
        // cosmos constructed with all needed dynamos
        Cosmos(RenderDynamo* renderDynamo, ControlDynamo* controlDynamo);   // etc...
        ~Cosmos();

        // indicate running status of cosmos to context
        // synchros can change behaviour based on cosmos' status
        // however, states only relevant to 1 synchro should not use this variable
        // EX: disabling user input should not override a transition or close state
        enum class Status {
            OK,                 // normal running status
            CLOSE,              // cosmos has finished and wishes to end
            TRANSITION,         // this cosmos wishes to end and switch to another
            FAIL,               // error has occurred and cannot be handled
        };

        // call all synchros to invoke system updates
        // return communicates if the cosmos has already finished and does not wish to be called
        // cosmos should also set/return a state enum indicating an exit state
        void update(double deltaTime);

        // check running state of Cosmos
        Cosmos::Status get_status();
        uint8_t get_status_value();
        // only Synchro friends set status, so no setter required

        // dynamically attach dynamo?
        // cosmos subclass can deal with it as they wish
        // (pass it to synchros who would "subscribe" to it)
        // otherwise just have static set of Dynamos on construction should be fine
        //void attach_dynamo();

    private:
        // use ECS (Entity, Component, Synchro) pattern to optimize update calls
        //std::unique_ptr<EntityRegistry>    m_entityRegistry;
        //std::unique_ptr<ComponentRegistry> m_componentRegistry;
        //std::unique_ptr<SynchroRegistry>   m_synchroRegistry; 

        // ECS synchros know their respective dynamos and feed components into them on update
        // synchros are created with their required dynamo (would dynamically attaching be useful?)
        // deleting or mutating a dynamo must apply to any synchros attached to it
        // to avoid dereferencing an invalid dynamo (done by CosmosManager?)
        // synchros need direct access to the ECS and some sort of access to dynamo
        
        // friendship to allow ECS access
        // synchros are treated as part of a Cosmos, they have full control
        friend class RenderSynchro;
        RenderSynchro*    m_renderSynch;

        friend class ControlSynchro;
        ControlSynchro*   m_controlSynch;

        //PhysicsSynchro*   m_physicsSynch;
        //AudioSynchro*     m_audioSynch;
        //NetSynchro*       m_netSynch;
        // etc...

        // Synchros will populate this to signal changes
        // NOTE: all synchros will share this one member?
        //   synchros will have to coordinate the highest priority value to set
        Status m_status = Status::OK;
        // indicates sub status
        uint8_t m_statusValue = 0;
    };
}

#endif // COSMOS_H