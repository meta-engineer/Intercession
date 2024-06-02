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
        // accumulated node transform from root node to first actual bone node
        glm::mat4 m_relativeTransform = glm::mat4(1.0f);

        // Name given for this armature
        std::string m_name;
        // Filepath this armature was imported from
        std::string m_sourceFilepath;
    };

    
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const Armature& data)
    {
        // REMEMBER this is a STACK so reverse the order!!!

        msg << data.m_sourceFilepath;
        msg << data.m_name;

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, Armature& data)
    {
        // only write data if name has changed.
        // if so fetch from ModelCache instead of deserializing?

        std::string newName;
        msg >> newName;

        std::string newSource;
        msg >> newSource;

        if (data.m_name != newName)
        {
            // try to import armature...
            data = ModelCache::fetch_armature(newName);
            // check if succeeded
            if (data.m_name != newName)
            {
                ModelCache::import(newSource);
                // try again?
                data = ModelCache::fetch_armature(newName);
            }
        }

        return msg;
    }
}

#endif // ARMATURE_H