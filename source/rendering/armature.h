#ifndef ARMATURE_H
#define ARMATURE_H

//#include "intercession_pch.h"
#include <vector>
#include <unordered_map>

#include "rendering/bone.h"
#include "events/message.h"

namespace pleep
{
    // Should match vertex shaders array max
    #define MAX_BONES 64

    // A collection of bones
    class Armature
    {
    public:
        // Init Armature with no bone data
        Armature() = default;

        // nodes/bones ordered by index (corresponding to vertex boneIds)
        std::vector<Bone> m_bones;

        // Name given for this armature
        std::string m_name;
        // Filepath this armature was imported from
        std::string m_sourceFilepath;
    };

    
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const Armature& data)
    {
        PLEEPLOG_DEBUG("Reached unimplemented Message stream in <Armature> overload!");
        // REMEMBER this is a STACK so reverse the order!!!

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, Armature& data)
    {
        PLEEPLOG_DEBUG("Reached unimplemented Message stream out <Armature> overload!");
        

        return msg;
    }
}

#endif // ARMATURE_H