#ifndef PROJECTILE_COMPONENT_H
#define PROJECTILE_COMPONENT_H

//#include "intercession_pch.h"

namespace pleep
{
    struct ProjectileComponent
    {
        double currLifetime = 0.0;
        double maxLifetime = 3.0;
    };
}

#endif // PROJECTILE_COMPONENT_H