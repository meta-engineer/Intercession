#ifndef ANIMATION_H
#define ANIMATION_H

#include <iostream>
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/scene.h>

// directly dependant on Model and its BoneInfo struct type
#include "model.h"
#include "bone.h"
#include "assimp_glm_convertors.h"

struct AssimpNodeData
{
    glm::mat4 transform;
    std::string name;
    int child_count;
    std::vector<AssimpNodeData> children;
};

class Animation
{
  public:
    Animation() = default;
    ~Animation();
    Animation(const std::string& path, Model* refreshModel = nullptr);

    Bone* findBone(const std::string& name);

    double getTicksPerSecond();
    double getDuration();
    const AssimpNodeData& getRootNode();
    const std::map<std::string, BoneInfo>& getBoneIDMap();

  private:
    void readMissingBones(const aiAnimation* animation, Model& model);
    void readHeirarchyData(AssimpNodeData& dest, const aiNode* src);

    double duration;
    double ticks_per_second;
    std::vector<Bone> bones;
    AssimpNodeData root_node;
    std::map<std::string, BoneInfo> bone_info_map;
};

Animation::~Animation() {}

Animation::Animation(const std::string& path, Model* refreshModel) 
{
    Assimp::Importer ai_importer;
    const aiScene* scene = ai_importer.ReadFile(path, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);

    // TODO: is this hardcoded to load the first animation?
    aiAnimation* animation = scene->mAnimations[0];

    duration = animation->mDuration;
    ticks_per_second = animation->mTicksPerSecond;

    readHeirarchyData(root_node, scene->mRootNode);

    // this is because "sometimes loading an FBX model separately some bones were missing and only found in the animation file"??
    // TODO: test this and determine if needed, or just bloat
    if (refreshModel)
        readMissingBones(animation, *refreshModel);
}

// its like find, but uses null to signal not found...
Bone* Animation::findBone(const std::string& name)
{
    // we're not in c land anymore...
    auto iter = std::find_if(bones.begin(), bones.end(),
        [&](const Bone& bone)
        {
            return bone.getBoneName() == name;
        }
    );

    if (iter == bones.end()) return nullptr;
    return &(*iter);
}

double Animation::getTicksPerSecond()
{
    return ticks_per_second;
}

double Animation::getDuration()
{
    return duration;
}

const AssimpNodeData& Animation::getRootNode()
{
    return root_node;
}

const std::map<std::string, BoneInfo>& Animation::getBoneIDMap()
{
    return bone_info_map;
}

glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

void Animation::readHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    // TODO: setup assertions along with project logger
    assert(src);

    dest.name = src->mName.data;
    dest.child_count = src->mNumChildren;
    // manually cast assimp math type to glm
    dest.transform = assimp_converters::convert_matrix(src->mTransformation);

    for (unsigned int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData new_data;
        readHeirarchyData(new_data, src->mChildren[i]);
        dest.children.push_back(new_data);
    }
}

void Animation::readMissingBones(const aiAnimation* animation, Model& model)
{
    unsigned int size = animation->mNumChannels;

    // fetch references to model class to update
    std::map<std::string, BoneInfo>& model_bone_info_map = model.getBoneInfoMap();
    int& model_bone_count = model.getBoneCount();

    for (unsigned int i = 0; i < size; i++)
    {
        aiNodeAnim* channel = animation->mChannels[i];
        std::string bone_name = channel->mNodeName.data;

        if (model_bone_info_map.find(bone_name) == model_bone_info_map.end())
        {
            model_bone_info_map[bone_name].ID = model_bone_count;
            model_bone_count++;
        }

        bones.push_back(
            Bone(channel->mNodeName.data,
                model_bone_info_map[channel->mNodeName.data].ID,
                channel)
        );
    }

    bone_info_map = model_bone_info_map;
}

#endif // ANIMATION_H