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
    double time_stamp;
};

struct KeyRotation
{
    glm::quat orientation;
    double time_stamp;
};

struct KeyScale
{
    glm::vec3 scale;
    double time_stamp;
};

class Bone
{
  public:
    // parse in keyframes from aiNodeAnim during animation construction
    Bone(const std::string& bone_name, int ID, const aiNodeAnim* channel);

    void update(double delta_time);

    glm::mat4 getLocalTransform();
    std::string getBoneName() const;
    int getBoneID();

    int getPositionIndex(double elapsed_time);
    int getRotationIndex(double elapsed_time);
    int getScaleIndex(double elapsed_time);

  private:
    float getInterpolateFactor(double start_time, double end_time, double elapsed_time);
    glm::mat4 interpolatePosition(double elapsed_time);
    glm::mat4 interpolateRotation(double elapsed_time);
    glm::mat4 interpolateScale(double elapsed_time);

    std::vector<KeyPosition> position_keys;
    std::vector<KeyRotation> rotation_keys;
    std::vector<KeyScale> scale_keys;

    int num_position_keys;
    int num_rotation_keys;
    int num_scale_keys;

    glm::mat4 local_transform;
    std::string name;
    int ID;
};


Bone::Bone(const std::string& bone_name, int ID, const aiNodeAnim* channel)
    : name(bone_name)
    , ID(ID)
    , local_transform(1.0f)
{
    num_position_keys = channel->mNumPositionKeys;
    for (int positionIndex = 0; positionIndex < num_position_keys; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        double time_stamp = channel->mPositionKeys[positionIndex].mTime;
        KeyPosition data;
        // manually cast assimp type
        data.position.x = aiPosition.x;
        data.position.y = aiPosition.y;
        data.position.z = aiPosition.z;
        data.time_stamp = time_stamp;
        position_keys.push_back(data);
    }

    num_rotation_keys = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < num_rotation_keys; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        double time_stamp = channel->mRotationKeys[rotationIndex].mTime;
        KeyRotation data;
        // manually cast assimp type
        // glm::quat constructor uses w,x,y,z ??? be careful!
        data.orientation.x = aiOrientation.x;
        data.orientation.y = aiOrientation.y;
        data.orientation.z = aiOrientation.z;
        data.orientation.w = aiOrientation.w;
        data.time_stamp = time_stamp;
        rotation_keys.push_back(data);
    }

    num_scale_keys = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < num_scale_keys; ++keyIndex)
    {
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        double time_stamp = channel->mScalingKeys[keyIndex].mTime;
        KeyScale data;
        // manually cast assimp type
        data.scale.x = scale.x;
        data.scale.y = scale.y;
        data.scale.z = scale.z;
        data.time_stamp = time_stamp;
        scale_keys.push_back(data);
    }
}

void Bone::update(double delta_time)
{
    glm::mat4 translation = interpolatePosition(delta_time);
    glm::mat4 rotation = interpolateRotation(delta_time);
    glm::mat4 scale = interpolateScale(delta_time);

    local_transform = translation * rotation * scale;
}

glm::mat4 Bone::getLocalTransform()
{
    return local_transform;
}

std::string Bone::getBoneName() const
{
    return name;
}

int Bone::getBoneID()
{
    return ID;
}

// get keyframe index of key with first time_stamp <= this time
// this is O(n) so not really ideal
int Bone::getPositionIndex(double elapsed_time)
{
    for (int index = 0; index < num_position_keys - 1; ++index)
    {
        if (elapsed_time < position_keys[index + 1].time_stamp)
            return index;
    }
    assert(false);
    return -1;
}

int Bone::getRotationIndex(double elapsed_time)
{
    for (int index = 0; index < num_rotation_keys - 1; ++index)
    {
        if (elapsed_time < rotation_keys[index + 1].time_stamp)
            return index;
    }
    assert(false);
    return -1;
}

int Bone::getScaleIndex(double elapsed_time)
{
    for (int index = 0; index < num_scale_keys - 1; ++index)
    {
        if (elapsed_time < scale_keys[index + 1].time_stamp)
            return index;
    }
    assert(false);
    return -1;
}

float Bone::getInterpolateFactor(double start_time, double end_time, double elapsed_time)
{
    double midway_length = elapsed_time - start_time;
    double frames_diff = end_time - start_time;
    float terp_factor = (float)(midway_length / frames_diff);
    // get a little non-linear
    return glm::smoothstep<float>(0.0f, 1.0f, terp_factor);
}

// get keys for before/after total elapsed animation time and interpolate
glm::mat4 Bone::interpolatePosition(double elapsed_time)
{
    if (1 == num_position_keys)
        return glm::translate(glm::mat4(1.0f), position_keys[0].position);

    int start_index = getPositionIndex(elapsed_time);
    int end_index   = start_index + 1;

    float terp_factor = getInterpolateFactor(
        position_keys[start_index].time_stamp,
        position_keys[end_index].time_stamp,
        elapsed_time
    );

    glm::vec3 final_position = glm::mix(
        position_keys[start_index].position,
        position_keys[end_index].position,
        terp_factor
    );

    return glm::translate(glm::mat4(1.0f), final_position);
}

glm::mat4 Bone::interpolateRotation(double elapsed_time)
{
    if (1 == num_rotation_keys)
        return glm::toMat4( glm::normalize(rotation_keys[0].orientation) );

    int start_index = getRotationIndex(elapsed_time);
    int end_index   = start_index + 1;

    float terp_factor = getInterpolateFactor(
        rotation_keys[start_index].time_stamp,
        rotation_keys[end_index].time_stamp,
        elapsed_time
    );

    glm::quat final_rotation = glm::slerp(
        rotation_keys[start_index].orientation,
        rotation_keys[end_index].orientation,
        terp_factor
    );

    return glm::toMat4( glm::normalize(final_rotation));
}

glm::mat4 Bone::interpolateScale(double elapsed_time)
{
    if (1 == num_scale_keys)
        return glm::scale(glm::mat4(1.0f), scale_keys[0].scale);

    int start_index = getScaleIndex(elapsed_time);
    int end_index   = start_index + 1;

    float terp_factor = getInterpolateFactor(
        scale_keys[start_index].time_stamp,
        scale_keys[end_index].time_stamp,
        elapsed_time
    );

    glm::vec3 final_scale = glm::mix(
        scale_keys[start_index].scale,
        scale_keys[end_index].scale,
        terp_factor
    );

    return glm::scale(glm::mat4(1.0f), final_scale);
}


#endif // BONE_H