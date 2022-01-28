#ifndef I_SYNCHRO_H
#define I_SYNCHRO_H

//#include "intercession_pch.h"
#include <set>

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
    class ISynchro
    {
    public:
        // Cosmos will call this universally for all synchros
        virtual void update(double deltaTime) = 0;

        std::set<Entity> m_entities;
    };
}

#endif // I_SYNCHRO_H