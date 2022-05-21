#ifndef I_CONTROL_COMPONENT_H
#define I_CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

namespace pleep
{
    // Vastly different required data merits a completely new component, however
    // A single component might want different behaviours but use the same data set
    // like meshes might want two different shaders...
    // We'll have each relay only excpect a single type (they only deal with business logic)
    // Dynamo's will accept and dispatch different types to different relays
    // We'll avoid as much inheritance & casting as possible while allowing the dispatch
    // (unlike IColliders who need to cross-polinate so casting is necessary)

    struct IControlComponent
    {
        // ***** Universal Controller Attributes *****
        // bool to ignore controller influence
        bool isActive = true;
    };
}

#endif // I_CONTROL_COMPONENT_H