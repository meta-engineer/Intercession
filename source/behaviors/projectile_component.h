#ifndef PROJECTILE_COMPONENT_H
#define PROJECTILE_COMPONENT_H

//#include "intercession_pch.h"

namespace pleep
{
    struct ProjectileComponent
    {
        double currLifetime = 0.0;
        double maxLifetime = 300.0;

        double hz = 1.0f;
        double scaleMax = 1.0f;
        double scaleMin = 1.0f;
    };
}

#endif // PROJECTILE_COMPONENT_H