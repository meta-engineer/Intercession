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

        

    private:
        // keyframes mapped by bone id
        std::unordered_map<int, std::vector<PositionKeyframe>> posKeyframes;
        std::unordered_map<int, std::vector<RotationKeyframe>> rotKeyframes;
        std::unordered_map<int, std::vector<ScaleKeyframe>>    sclKeyframes;

        double duration;
        double frequency; // "ticks" per second (what does assimp define as tick? some arbitrary time scale)

        
        // Name given for this animation
        std::string m_name;
        // Filename this animation was imported from
        std::string m_sourceFilename;
    };
}

#endif // ANIMATION_SKELETAL_H