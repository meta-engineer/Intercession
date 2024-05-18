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
        std::string m_currentAnimation = "";
        // length of time since current animation started (seconds)
        double m_currentTime = 0;

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

        // for each animation store name and path
        for (auto& anime : data.animations)
        {
            msg << anime.first;
            msg << anime.second->m_sourceFilepath;
        }
        msg << data.animations.size();

        // Since server doesn't have RednerDynamo time will always be 0, so leave it as a client-side only member.
        //msg << data.m_currentTime;
        msg << data.m_currentAnimation;

        return msg;
    }
    template<typename T_Msg>
    Message<T_Msg>& operator>>(Message<T_Msg>& msg, AnimationComponent& data)
    {
        // Deserialize each animation?
        // If name does not exist in local map then fetch it using path
        msg >> data.m_currentAnimation;
        //msg >> data.m_currentTime;

        size_t numAnime;
        msg >> numAnime;
        for (size_t i = 0; i < numAnime; i++)
        {
            std::string animeSource;
            msg >> animeSource;
            std::string animeName;
            msg >> animeName;

            if (data.animations.count(animeName) == 0)
            {
                // try to import it
                std::shared_ptr<const AnimationSkeletal> newAnime = ModelCache::fetch_animation(animeName);

                if (newAnime == nullptr)
                {
                    ModelCache::import(animeSource);
                    newAnime = ModelCache::fetch_animation(animeName);
                }
                
                // if success then put into map
                if (newAnime != nullptr)
                {
                    data.animations[animeName] = newAnime;
                }
            }
        }

        return msg;
    }
}

#endif // ANIMATION_COMPONENT_H