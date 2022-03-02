#ifndef CONTROL_COMPONENT_H
#define CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Control system needs to transfer the callbacks in ControlDynamo
    // into changes in other components (transform, physics, audio, etc...)
    // ControlSynchro can't know which auxillary components to sign for
    // once its passed to dynamo, it no longer has access to other components
    // Uniquely the control relay will receive a pointer to the cosmos
    // which owns the entity for this component
    // this will allow it to access the whole ecs however it needs

    struct ControlComponent
    {
        bool active = true;
    };
}

#endif // CONTROL_COMPONENT_H