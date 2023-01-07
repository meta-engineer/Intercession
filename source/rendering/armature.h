#ifndef ARMATURE_H
#define ARMATURE_H

//#include "intercession_pch.h"
#include <vector>
#include <unordered_map>

#include "rendering/bone.h"

namespace pleep
{
    // A collection of bones
    class Armature
    {
    public:
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
}

#endif // ARMATURE_H