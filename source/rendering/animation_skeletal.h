#ifndef ANIMATION_SKELETAL_H
#define ANIMATION_SKELETAL_H

//#include "intercession_pch.h"
#include <unordered_map>

#include "physics/transform_component.h"

namespace pleep
{
    // Single keyframe for 3d transform data (position, orientation, scale)
    // Borrows TransformComponent
    /*
    struct TransformKeyframe
    {
        TransformComponent transform;
        double timeStamp;
    };
    // assimp stores each transform component as seperate keyframes.
    // we could probably interpolate them on load and flatten it into one set of TrasformKeyframes,
    // but for now we'll just use them as is, and we can refactor it if necessary later
    */

    // position, rotation, scale all be keyframed seperately:
    struct PositionKeyframe
    {
        glm::vec3 position;
        double timeStamp;
    };

    struct RotationKeyframe
    {
        glm::quat orientation;
        double timeStamp;
    };

    struct ScaleKeyframe
    {
        glm::vec3 scale;
        double timeStamp;
    };

    // Skeleton Animation data
    class AnimationSkeletal
    {
    public:
        AnimationSkeletal() = default;
        ~AnimationSkeletal() = default;

        double m_duration;  // in units of "ticks"
        double m_frequency; // "ticks" per second (to convert real-time to animation-time)

        // keyframes mapped by boneId
        std::unordered_map<unsigned int, std::vector<PositionKeyframe>> m_posKeyframes;
        std::unordered_map<unsigned int, std::vector<RotationKeyframe>> m_rotKeyframes;
        std::unordered_map<unsigned int, std::vector<ScaleKeyframe>>    m_sclKeyframes;

        // Name given for this animation
        std::string m_name;
        // Filename this animation was imported from
        std::string m_sourceFilepath;
    };
}

#endif // ANIMATION_SKELETAL_H