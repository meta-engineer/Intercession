#ifndef I_COLLIDER_H
#define I_COLLIDER_H

//#include "intercession_pch.h"
#include <memory>

namespace pleep
{
    // we want a singular class pointer to store in component,
    // but we also need specific data individual to each type,
    // so we need to be able to case them...
    // the only way would know is by uniquely identifying with an enum

    enum ColliderType
    {
        none,
        AABB,
        box,
        sphere,
        mesh
    };

    class ICollider
    {
    public:
        // type should be none if subclass forgets to define it
        ICollider(const ColliderType thisType = ColliderType::none)
            : type(thisType)
        {}

        const ColliderType type;

        // Derived colliders should cast other appropriately and collider
        // or throw error that collider of other's type is not implemented
        virtual bool intersects(std::shared_ptr<ICollider>& other) = 0;
    private:
        
    };
}

#endif // I_COLLIDER_H