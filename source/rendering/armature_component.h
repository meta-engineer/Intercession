#ifndef ARMATURE_COMPONENT_H
#define ARMATURE_COMPONENT_H

//#include "intercession_pch.h"
#include <vector>

#include "events/message.h"
#include "rendering/armature.h"

namespace pleep
{
    struct ArmatureComponent
    {
        std::shared_ptr<Armature> armature;
    };


    // Becuase Bone data is individual to each entity serialization cannot just reference
    // a key in the model library. (though it could reference it for size/structure?)
    // So we may have to actually serialize it.
    // Though it shouldn't be as large as mesh data, or involve gpu memory like materials
    // so it shouldn't be too expensive to serialize.
    // Armature could also have a dirty bit to help reduce cost?

    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const ArmatureComponent& data)
    {
        // forward serializer to armature? or do it here?
        msg << *(data.armature);

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, ArmatureComponent& data)
    {
        // forward deserializer to armature
        if (data.armature == nullptr) data.armature = std::make_shared<Armature>();
        msg >> *(data.armature);

        return msg;
    }
}

#endif // ARMATURE_COMPONENT_H