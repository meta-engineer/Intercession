#ifndef I_SYNCHRO_H
#define I_SYNCHRO_H

//#include "intercession_pch.h"
#include <set>
#include <memory>

#include "ecs_types.h"


namespace pleep
{
    /*
    // Usage for a synchro derivation:
    for (Entity const& entity : m_entities)
    {
        RigidBody& rigidBody   = componentRegistry.get_component<RigidBody>(entity);
        Transform& transform   = componentRegistry.get_component<Transform>(entity);
        Gravity const& gravity = componentRegistry.get_component<Gravity>(entity);

        // modify component references
        ...
    }
    */
   
    // forward declare parent/owner
    class Cosmos;

    // Interface for synchros to be invoked by cosmos
    class I_Synchro
    {
    public:
        // subclasses should "using I_Synchro::I_Synchro" or overload with constructor which calls here
        I_Synchro(std::weak_ptr<Cosmos> owner)
            : m_ownerCosmos(owner)
        {}
        virtual ~I_Synchro() = default;

        // Cosmos will call this universally for all synchros
        virtual void update() = 0;

        // Each subclass can suggest to registry what component signature it requires derived from known cosmos component set
        // returns empty bitset if desired components could not be found
        virtual Signature derive_signature() = 0;

        // entities to fetch from owner cosmos and feed to dynamo
        // set by SynchroRegistry
        std::set<Entity> m_entities;

    protected:
        // Access to ecs where m_entities are contained
        // weak to avoid circular smart pointer with cosmos, 
        // deleting context's pointer should delete us (after all dynamos safely are cleared)
        std::weak_ptr<Cosmos> m_ownerCosmos;
    };
}

#endif // I_SYNCHRO_H