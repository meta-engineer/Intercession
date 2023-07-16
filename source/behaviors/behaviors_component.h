#ifndef BEHAVIORS_COMPONENT_H
#define BEHAVIORS_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>
#include <typeinfo>

#include "behaviors/behaviors_library.h"
#include "behaviors/i_behaviors_drivetrain.h"
#include "events/message.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // Contains a behaviors interface which dispatches to assigned drivetrain
    // This doesn't pack into the ECS, but since behaviors subclasses are user defined we
    //   cannot account for them all with individual synchros
    struct BehaviorsComponent
    {
        std::shared_ptr<I_BehaviorsDrivetrain> drivetrain = nullptr;

        // enable methods of this behaviors drivetrain for the entity with this BehaviorsComponent
        // each flag should mirror the structure of I_BehaviorsDrivetrain
        bool use_fixed_update       = false;
        bool use_frame_update       = false;
        // using collision will be selected by the I_Collider
        //bool use_collision          = false;
        //bool use_collision_enter    = false;
        //bool use_collision_exit    = false;
    };
    
    // Member pointers makes BehaviorsComponent not sharable, so we must override Message serialization
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const BehaviorsComponent& data)
    {
        msg << (data.drivetrain ? data.drivetrain->m_libraryType : BehaviorsLibrary::BehaviorsType::none);
        msg << data.use_fixed_update;
        msg << data.use_frame_update;
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, BehaviorsComponent& data)
    {
        msg >> data.use_frame_update;
        msg >> data.use_fixed_update;

        BehaviorsLibrary::BehaviorsType behaviorsType;
        msg >> behaviorsType;
        // if drivetrain does not exist OR drivetrain exists and type does not match
        // (do not realloc memory for the same behaviors drivetrain)
        // library will return nullptr if type is none
        if (!data.drivetrain || data.drivetrain->m_libraryType != behaviorsType)
        {
            data.drivetrain = BehaviorsLibrary::fetch_behaviors(behaviorsType);
        }
        return msg;
    }
}

#endif // BEHAVIORS_COMPONENT_H