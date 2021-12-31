#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "animation.h"
#include "bone.h"

class Animator
{
  public:
    Animator();

    void update_animation(double deltaTime);
    void play_animation(Animation* anim);
    void calculate_bone_transform(const AssimpNodeData* node, glm::mat4 parentTransform);
    std::vector<glm::mat4> get_final_bone_matrices();

  private:
    std::vector<glm::mat4> finalBoneMatrices;
    Animation* currAnimation;
    double currTime;
    double lastDeltaTime;  // allocate memory here so its not done every frame?
};


Animator::Animator()
    : currTime(0.0)
    , currAnimation(nullptr)
{
    finalBoneMatrices.reserve(100);

    for (int i = 0; i < 100; i++)
        finalBoneMatrices.push_back(glm::mat4(1.0f));
}

void Animator::update_animation(double deltaTime)
{
    lastDeltaTime = deltaTime;

    if (currAnimation)
    {
        currTime += currAnimation->get_ticks_per_second() * deltaTime;
        currTime = fmod(currTime, currAnimation->get_duration());

        calculate_bone_transform(&currAnimation->get_root_node(), glm::mat4(1.0f));
    }
}

void Animator::play_animation(Animation* anim)
{
    currAnimation = anim;
    currTime = 0.0f;
}

void Animator::calculate_bone_transform(const AssimpNodeData* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transform;

    Bone* bone = currAnimation->find_bone(nodeName);

    if (bone)
    {
        bone->update(currTime);
        nodeTransform = bone->get_local_transform();
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    std::map<std::string, BoneInfo> animBoneInfoMap = currAnimation->get_bone_id_map();
    if (animBoneInfoMap.find(nodeName) != animBoneInfoMap.end())
    {
        int index = animBoneInfoMap[nodeName].id;
        glm::mat4 model_to_bone = animBoneInfoMap[nodeName].model_to_bone;
        finalBoneMatrices[index] = globalTransform * model_to_bone;
    }

    for (int i = 0; i < node->numChildren; i++)
    {
        calculate_bone_transform(&node->children[i], globalTransform);
    }
}

std::vector<glm::mat4> Animator::get_final_bone_matrices()
{
    return finalBoneMatrices;
}


#endif // ANIMATOR_H