#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "animation.h"
#include "bone.h"

class Animator
{
  public:
    Animator();

    void updateAnimation(double delta_time);
    void playAnimation(Animation* anim);
    void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parent_transform);
    std::vector<glm::mat4> getFinalBoneMatrices();

  private:
    std::vector<glm::mat4> final_bone_matrices;
    Animation* curr_animation;
    double curr_time;
    double last_delta_time;  // allocate memory here so its not done every frame?
};


Animator::Animator()
    : curr_time(0.0)
    , curr_animation(nullptr)
{
    final_bone_matrices.reserve(100);

    for (int i = 0; i < 100; i++)
        final_bone_matrices.push_back(glm::mat4(1.0f));
}

void Animator::updateAnimation(double delta_time)
{
    last_delta_time = delta_time;

    if (curr_animation)
    {
        curr_time += curr_animation->getTicksPerSecond() * delta_time;
        curr_time = fmod(curr_time, curr_animation->getDuration());

        calculateBoneTransform(&curr_animation->getRootNode(), glm::mat4(1.0f));
    }
}

void Animator::playAnimation(Animation* anim)
{
    curr_animation = anim;
    curr_time = 0.0f;
}

void Animator::calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parent_transform)
{
    std::string node_name = node->name;
    glm::mat4 node_transform = node->transform;

    Bone* bone = curr_animation->findBone(node_name);

    if (bone)
    {
        bone->update(curr_time);
        node_transform = bone->getLocalTransform();
    }

    glm::mat4 global_transform = parent_transform * node_transform;

    std::map<std::string, BoneInfo> bone_info_map = curr_animation->getBoneIDMap();
    if (bone_info_map.find(node_name) != bone_info_map.end())
    {
        int index = bone_info_map[node_name].ID;
        glm::mat4 model_to_bone = bone_info_map[node_name].model_to_bone;
        final_bone_matrices[index] = global_transform * model_to_bone;
    }

    for (int i = 0; i < node->child_count; i++)
    {
        calculateBoneTransform(&node->children[i], global_transform);
    }
}

std::vector<glm::mat4> Animator::getFinalBoneMatrices()
{
    return final_bone_matrices;
}


#endif // ANIMATOR_H