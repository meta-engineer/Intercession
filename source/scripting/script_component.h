#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>

#include "scripting/i_script_drivetrain.h"
#include "events/message.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Contains a script interface which dispatches to assigned handlers
    // This doesn't pack into the ECS, but since script subclasses are user defined we
    //   cannot account for them all with individual synchros
    struct ScriptComponent
    {
        std::shared_ptr<I_ScriptDrivetrain> handlers;
    };
    
    // Member pointers makes ScriptComponent not sharable, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const ScriptComponent& data)
    {
        PLEEPLOG_DEBUG("Reached unimplemented Message stream in <ScriptComponent> overload!");
        UNREFERENCED_PARAMETER(data);
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, ScriptComponent& data)
    {
        PLEEPLOG_DEBUG("Reached unimplemented Message stream out <ScriptComponent> overload!");
        UNREFERENCED_PARAMETER(data);
        return msg;
    }
}

#endif // SCRIPT_COMPONENT_H