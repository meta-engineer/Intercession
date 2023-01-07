#ifndef ARMATURE_COMPONENT_H
#define ARMATURE_COMPONENT_H

//#include "intercession_pch.h"
#include <vector>

#include "events/message.h"

namespace pleep
{
    struct ArmatureComponent
    {
        //std::vector<Bone> bones;
        // TODO: build bone heirarchy directly into ecs?
        //std::unordered_map<std::string, BoneInfo> boneInfoMap
        //TransformNode rootNode;
    };

    // Becuase Bone data is individual to each entity serialization cannot just reference
    // a key in the model library. (though it could reference it for size/structure?)
    // So we may have to actually serialize it.
    // Though it shouldn't be as large as mesh data, and it doesn't involve gpu memory like materials
    // so it shouldn't be too expensive to serialize.
    // Armature could also have a dirty bit to help reduce cost?

    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const ArmatureComponent& data)
    {
        // forward serializer to armature
        msg << data.armature;

        /*
        uint32_t i = static_cast<uint32_t>(msg.size());
        // resize all at once
        msg.body.resize(msg.body.size() + I_ColliderComponent::dataSize);

        std::memcpy(msg.body.data() + i, &(data.responseType), sizeof(CollisionResponseType));
        i += sizeof(CollisionResponseType);
        
        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
        */
        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, ArmatureComponent& data)
    {
        // forward deserializer to armature
        msg >> data.armature;

        /*
        // stream out when no data is available;
        assert(msg.size() >= I_ColliderComponent::dataSize);
        
        // track index at the start of the data on "top" of the stack
        uint32_t i = static_cast<uint32_t>(msg.size()) - I_ColliderComponent::dataSize;
        
        std::memcpy(&(data.responseType), msg.body.data() + i, sizeof(CollisionResponseType));
        i += sizeof(CollisionResponseType);
        
        // shrink, removing end of stack (constant time)
        msg.body.resize(msg.size() - I_ColliderComponent::dataSize);

        // recalc message size
        msg.header.size = static_cast<uint32_t>(msg.size());
        */
        return msg;
    }
}

#endif // ARMATURE_COMPONENT_H