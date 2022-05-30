#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>
#include "scripting/i_script_drivetrain.h"

namespace pleep
{
    // Contains a script interface which dispatches to assigned handlers
    // This doesn't pack into the ECS, but since script subclasses are user defined we
    //   cannot account for them all with individual synchros
    struct ScriptComponent
    {
        std::shared_ptr<IScriptDrivetrain> handlers;
    };
}

#endif // SCRIPT_COMPONENT_H