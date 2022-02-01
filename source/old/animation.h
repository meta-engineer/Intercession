#ifndef ANIMATION_H
#define ANIMATION_H

#include <iostream>
#include <vector>
#include <string>
#include <cassert>

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
    int numChildren;
    std::vector<AssimpNodeData> children;
};

class Animation
{
  public:
    Animation() = default;
    ~Animation();
    Animation(const std::string& path, Model* refreshModel = nullptr);

    Bone* find_bone(const std::string& name);

    double get_ticks_per_second();
    double get_duration();
    const AssimpNodeData& get_root_node();
    const std::map<std::string, BoneInfo>& get_bone_id_map();

  private:
    void _read_missing_bones(const aiAnimation* animation, Model& model);
    void _read_heirarchy_data(AssimpNodeData& dest, const aiNode* src);
    
    double duration;
    double ticksPerSecond;
    std::vector<Bone> bones;
    AssimpNodeData rootNode;
    std::map<std::string, BoneInfo> boneInfoMap;
};

Animation::~Animation() {}

Animation::Animation(const std::string& path, Model* refreshModel) 
{
    Assimp::Importer aiImporter;
    const aiScene* scene = aiImporter.ReadFile(path, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);

    // TODO: is this hardcoded to load the first animation?
    aiAnimation* animation = scene->mAnimations[0];

    duration = animation->mDuration;
    ticksPerSecond = animation->mTicksPerSecond;

    _read_heirarchy_data(rootNode, scene->mRootNode);

    // this is because "sometimes loading an FBX model separately some bones were missing and only found in the animation file"??
    // TODO: test this and determine if needed, or just bloat
    if (refreshModel)
        _read_missing_bones(animation, *refreshModel);
}

// its like find, but uses null to signal not found...
Bone* Animation::find_bone(const std::string& name)
{
    // we're not in c land anymore...
    auto iter = std::find_if(bones.begin(), bones.end(),
        [&](const Bone& bone)
        {
            return bone.get_bone_name() == name;
        }
    );

    if (iter == bones.end()) return nullptr;
    return &(*iter);
}

double Animation::get_ticks_per_second()
{
    return ticksPerSecond;
}

double Animation::get_duration()
{
    return duration;
}

const AssimpNodeData& Animation::get_root_node()
{
    return rootNode;
}

const std::map<std::string, BoneInfo>& Animation::get_bone_id_map()
{
    return boneInfoMap;
}

void Animation::_read_heirarchy_data(AssimpNodeData& dest, const aiNode* src)
{
    // TODO: setup assertions along with project logger
    assert(src);

    dest.name = src->mName.data;
    dest.numChildren = src->mNumChildren;
    // manually cast assimp math type to glm
    dest.transform = assimp_converters::convert_matrix(src->mTransformation);

    for (unsigned int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData childData;
        _read_heirarchy_data(childData, src->mChildren[i]);
        dest.children.push_back(childData);
    }
}

void Animation::_read_missing_bones(const aiAnimation* animation, Model& model)
{
    unsigned int size = animation->mNumChannels;

    // fetch references to model class to update
    std::map<std::string, BoneInfo>& model_boneInfoMap = model.get_bone_info_map();
    int& model_boneCount = model.get_num_bones();

    for (unsigned int i = 0; i < size; i++)
    {
        aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (model_boneInfoMap.find(boneName) == model_boneInfoMap.end())
        {
            model_boneInfoMap[boneName].id = model_boneCount;
            model_boneCount++;
        }

        bones.push_back(
            Bone(channel->mNodeName.data,
                model_boneInfoMap[channel->mNodeName.data].id,
                channel)
        );
    }

    boneInfoMap = model_boneInfoMap;
}

#endif // ANIMATION_H