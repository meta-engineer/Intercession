#ifndef BONE_H
#define BONE_H

//#include "intercession_pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "events/message.h"

namespace pleep
{
    // Defines a sub-space of the cosmos as part of an armature
    // Armatures contain a list of Bones indexed by id
    // So Bones should store their children as a list of ids
    class Bone
    {
    public:
        Bone() = default;
        Bone(const std::string& name, const int id, const glm::mat4& relativeTransform)
            : m_name(name)
            , m_id(id)
            , m_relativeTransform(relativeTransform)
        {}
        ~Bone() = default;
        
        // the current transform position/orientation of this bone (modified by AnimationSkeletal)
        glm::mat4 m_localTransform = glm::mat4(1.0f);


        // "const" members set at import time:

        // transform relative to this node's parent
        glm::mat4 m_relativeTransform = glm::mat4(1.0f);
        // transform relative to model space (inverse bind pose matrix) aka model_to_bone
        glm::mat4 m_bindTransform = glm::mat4(1.0f);

        // Name should NOT be used by animations/armature after importing, just for convenience
        std::string m_name;
        // mine and my children's ids match indexed position in Armature
        unsigned int m_id;
        std::vector<unsigned int> m_childIds;
    };
}

#endif // BONE_H