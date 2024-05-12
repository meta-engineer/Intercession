#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

//#include "intercession_pch.h"
#include <memory>
#include <unordered_map>
#include <string>

#include "events/message.h"
#include "rendering/animation_skeletal.h"

namespace pleep
{
    class AnimationComponent
    {
    public:
        // Store all state for current animation:

        // should have matching entry in animations
        std::string m_currentAnimation;
        // length of time since current animation started
        double m_currentTime;

        // animation name -> reference to shared cached animation
        // all animations be stored in one component?
        // do animations need to be mutable...? or can we have references to ModelManager data?
        std::unordered_map<std::string, std::shared_ptr<const AnimationSkeletal>> animations;
    };
    
    template<typename T_Msg>
    Message<T_Msg>& operator<<(Message<T_Msg>& msg, const AnimationComponent& data)
    {
        // Serialize each animation?
        // Write number in map and each name/path
        UNREFERENCED_PARAMETER(msg);
        UNREFERENCED_PARAMETER(data);

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, AnimationComponent& data)
    {
        // Deserialize each animation?
        // If name does not exist in local map then fetch it using path
        UNREFERENCED_PARAMETER(msg);
        UNREFERENCED_PARAMETER(data);

        return msg;
    }
}

#endif // ANIMATION_COMPONENT_H