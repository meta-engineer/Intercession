#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>
#include <typeinfo>

#include "scripting/script_library.h"
#include "scripting/i_script_drivetrain.h"
#include "events/message.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Contains a script interface which dispatches to assigned drivetrain
    // This doesn't pack into the ECS, but since script subclasses are user defined we
    //   cannot account for them all with individual synchros
    struct ScriptComponent
    {
        std::shared_ptr<I_ScriptDrivetrain> drivetrain = nullptr;

        // enable methods of this script drivetrain for the entity with this ScriptComponent
        // each flag should mirror the structure of I_ScriptDrivetrain
        bool use_fixed_update       = false;
        bool use_frame_update       = false;
        bool use_collision          = false;
        //bool use_collision_enter    = false;
        //bool use_collision_exit    = false;
    };
    
    // Member pointers makes ScriptComponent not sharable, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const ScriptComponent& data)
    {
        msg << (data.drivetrain ? data.drivetrain->m_libraryType : ScriptLibrary::ScriptType::none);
        msg << data.use_fixed_update;
        msg << data.use_frame_update;
        msg << data.use_collision;
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, ScriptComponent& data)
    {
        msg >> data.use_collision;
        msg >> data.use_frame_update;
        msg >> data.use_fixed_update;

        ScriptLibrary::ScriptType scriptType;
        msg >> scriptType;
        // if drivetrain does not exist OR drivetrain exists and type does not match
        // (do not realloc memory for the same script drivetrain)
        // library will return nullptr if type is none
        if (!data.drivetrain || data.drivetrain->m_libraryType != scriptType)
        {
            data.drivetrain = ScriptLibrary::fetch_script(scriptType);
        }
        return msg;
    }
}

#endif // SCRIPT_COMPONENT_H