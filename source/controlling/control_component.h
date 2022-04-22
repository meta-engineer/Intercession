#ifndef CONTROL_COMPONENT_H
#define CONTROL_COMPONENT_H

//#include "intercession_pch.h"
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include "logging/pleep_log.h"
#include "ecs/ecs_types.h"

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

        // TODO: indicate what kind of ControlRelay I should be managed by
        // This likely should be using a RelayLibrary enum

        // Relays are the functionality, the component should be the data...
        // But, how to store variables specific to each type of controller?
        // can ALL controller types share the same component?
        // should each component have an interface that dispatches according to controller type?

        // track another entity for my behaviour
        Entity target = NULL_ENTITY;
        // location in target local space to track (their head for example)
        // should this be stored & set by that entity instead?
        glm::vec3 targetOffset = glm::vec3(0.0f);
        // relative location to allow dynamic framing of target
        glm::vec3 dynamicOffset     = glm::vec3(0.0f);
        float maxDynamicOffset      = 0.0f;
        // range is along my transform heading
        float range                 = 8.0f;
        float maxRange              = 10.0f;
    };
}

#endif // CONTROL_COMPONENT_H