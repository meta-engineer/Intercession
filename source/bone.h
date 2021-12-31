#ifndef BONE_H
#define BONE_H

#include <iostream>
#include <vector>
#include <string>

// TODO: pre-compiled header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>

// position, rotation, scale can all be keyframed seperately
struct KeyPosition
{
    glm::vec3 position;
    double timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    double timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    double timeStamp;
};

class Bone
{
  public:
    // parse in keyframes from aiNodeAnim during animation construction
    Bone(const std::string& boneName, int id, const aiNodeAnim* channel);

    void update(double deltaTime);

    glm::mat4 get_local_transform();
    std::string get_bone_name() const;
    int get_bone_id();

    int get_position_index(double elapsedTime);
    int get_rotation_index(double elapsedTime);
    int get_scale_index(double elapsedTime);

  private:
    float     _get_interpolate_factor(double startTime, double endTime, double elapsedTime);
    glm::mat4 _interpolate_position(double elapsedTime);
    glm::mat4 _interpolate_rotation(double elapsedTime);
    glm::mat4 _interpolate_scale(double elapsedTime);

    std::vector<KeyPosition> positionKeys;
    std::vector<KeyRotation> rotationKeys;
    std::vector<KeyScale>    scaleKeys;

    int numPositionKeys;
    int numRotationKeys;
    int numScaleKeys;

    glm::mat4 localTransform;
    std::string name;
    int id;
};


Bone::Bone(const std::string& boneName, int id, const aiNodeAnim* channel)
    : name(boneName)
    , id(id)
    , localTransform(1.0f)
{
    numPositionKeys = channel->mNumPositionKeys;
    for (int positionIndex = 0; positionIndex < numPositionKeys; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        double timeStamp = channel->mPositionKeys[positionIndex].mTime;
        KeyPosition data;
        // manually cast assimp type
        data.position.x = aiPosition.x;
        data.position.y = aiPosition.y;
        data.position.z = aiPosition.z;
        data.timeStamp = timeStamp;
        positionKeys.push_back(data);
    }

    numRotationKeys = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < numRotationKeys; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        double timeStamp = channel->mRotationKeys[rotationIndex].mTime;
        KeyRotation data;
        // manually cast assimp type
        // glm::quat constructor uses w,x,y,z ??? be careful!
        data.orientation.x = aiOrientation.x;
        data.orientation.y = aiOrientation.y;
        data.orientation.z = aiOrientation.z;
        data.orientation.w = aiOrientation.w;
        data.timeStamp = timeStamp;
        rotationKeys.push_back(data);
    }

    numScaleKeys = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < numScaleKeys; ++keyIndex)
    {
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        double timeStamp = channel->mScalingKeys[keyIndex].mTime;
        KeyScale data;
        // manually cast assimp type
        data.scale.x = scale.x;
        data.scale.y = scale.y;
        data.scale.z = scale.z;
        data.timeStamp = timeStamp;
        scaleKeys.push_back(data);
    }
}

void Bone::update(double deltaTime)
{
    glm::mat4 translation = _interpolate_position(deltaTime);
    glm::mat4 rotation = _interpolate_rotation(deltaTime);
    glm::mat4 scale = _interpolate_scale(deltaTime);

    localTransform = translation * rotation * scale;
}

glm::mat4 Bone::get_local_transform()
{
    return localTransform;
}

std::string Bone::get_bone_name() const
{
    return name;
}

int Bone::get_bone_id()
{
    return id;
}

// get keyframe index of key with first timeStamp <= this time
// this is O(n) so not really ideal
int Bone::get_position_index(double elapsedTime)
{
    for (int index = 0; index < numPositionKeys - 1; ++index)
    {
        if (elapsedTime < positionKeys[index + 1].timeStamp)
            return index;
    }
    assert(false);
    return -1;
}

int Bone::get_rotation_index(double elapsedTime)
{
    for (int index = 0; index < numRotationKeys - 1; ++index)
    {
        if (elapsedTime < rotationKeys[index + 1].timeStamp)
            return index;
    }
    assert(false);
    return -1;
}

int Bone::get_scale_index(double elapsedTime)
{
    for (int index = 0; index < numScaleKeys - 1; ++index)
    {
        if (elapsedTime < scaleKeys[index + 1].timeStamp)
            return index;
    }
    assert(false);
    return -1;
}

float Bone::_get_interpolate_factor(double startTime, double endTime, double elapsedTime)
{
    double midwayTime = elapsedTime - startTime;
    double frameDelta = endTime - startTime;
    float terpFactor = (float)(midwayTime / frameDelta);
    // get a little non-linear
    return glm::smoothstep<float>(0.0f, 1.0f, terpFactor);
}

// get keys for before/after total elapsed animation time and interpolate
glm::mat4 Bone::_interpolate_position(double elapsedTime)
{
    if (1 == numPositionKeys)
        return glm::translate(glm::mat4(1.0f), positionKeys[0].position);

    int startIndex = get_position_index(elapsedTime);
    int endIndex   = startIndex + 1;

    float terpFactor = _get_interpolate_factor(
        positionKeys[startIndex].timeStamp,
        positionKeys[endIndex].timeStamp,
        elapsedTime
    );

    glm::vec3 final_position = glm::mix(
        positionKeys[startIndex].position,
        positionKeys[endIndex].position,
        terpFactor
    );

    return glm::translate(glm::mat4(1.0f), final_position);
}

glm::mat4 Bone::_interpolate_rotation(double elapsedTime)
{
    if (1 == numRotationKeys)
        return glm::toMat4( glm::normalize(rotationKeys[0].orientation) );

    int startIndex = get_rotation_index(elapsedTime);
    int endIndex   = startIndex + 1;

    float terpFactor = _get_interpolate_factor(
        rotationKeys[startIndex].timeStamp,
        rotationKeys[endIndex].timeStamp,
        elapsedTime
    );

    glm::quat final_rotation = glm::slerp(
        rotationKeys[startIndex].orientation,
        rotationKeys[endIndex].orientation,
        terpFactor
    );

    return glm::toMat4( glm::normalize(final_rotation));
}

glm::mat4 Bone::_interpolate_scale(double elapsedTime)
{
    if (1 == numScaleKeys)
        return glm::scale(glm::mat4(1.0f), scaleKeys[0].scale);

    int startIndex = get_scale_index(elapsedTime);
    int endIndex   = startIndex + 1;

    float terpFactor = _get_interpolate_factor(
        scaleKeys[startIndex].timeStamp,
        scaleKeys[endIndex].timeStamp,
        elapsedTime
    );

    glm::vec3 final_scale = glm::mix(
        scaleKeys[startIndex].scale,
        scaleKeys[endIndex].scale,
        terpFactor
    );

    return glm::scale(glm::mat4(1.0f), final_scale);
}


#endif // BONE_H