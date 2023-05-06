#ifndef ARMATURE_H
#define ARMATURE_H

//#include "intercession_pch.h"
#include <vector>
#include <unordered_map>

#include "rendering/bone.h"
#include "events/message.h"

namespace pleep
{
    // A collection of bones
    class Armature
    {
    public:
        // Init Armature with no bone data
        Armature() = default;
        Armature(const std::vector<Bone>& bones, const std::unordered_map<std::string, unsigned int>& boneIdMap)
            : m_bones(bones)
            , m_boneIdMap(boneIdMap)
        {}
        ~Armature() = default;

        // nodes/bones ordered by index (corresponding to vertex boneIds)
        std::vector<Bone> m_bones;
        // map bone names to id (mostly for animation importing)
        std::unordered_map<std::string, unsigned int> m_boneIdMap;
        
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