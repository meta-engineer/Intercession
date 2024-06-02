#ifndef ANIMATION_RELAY_H
#define ANIMATION_RELAY_H

//#include "intercession_pch.h"
#include "rendering/animation_packet.h"

namespace pleep
{
    // Not inheriting from A_RenderRelay because it doesn't really have the same fingerprint
    class AnimationRelay
    {
    public:
        AnimationRelay() = default;
        ~AnimationRelay() = default;

        void engage(double deltaTime)
        {
            UNREFERENCED_PARAMETER(deltaTime);
            // for each armature, update all its bones using deltaTime
            // What about dynamic animations? based on walking distance/speed for example?
            // some sort of event based system to connect biped behaviours and animations?
            for (std::vector<AnimationPacket>::iterator packet_it = m_animationPackets.begin(); packet_it != m_animationPackets.end(); packet_it++)
            {
                AnimationPacket& data = *packet_it;

                if (data.animatable.m_currentAnimation == "")
                {
                    clear_bones_transform(data.renderable.armature.m_bones);
                    continue;
                }

                auto animation_it = data.animatable.animations.find(data.animatable.m_currentAnimation);
                if (animation_it == data.animatable.animations.end())
                {
                    PLEEPLOG_WARN("Current animation \"" + data.animatable.m_currentAnimation + "\" does not exist in animation map? Disabling current animation...");
                    data.animatable.m_currentAnimation = "";
                    continue;
                }

                // step time forwards
                // TODO: we want this to be tied to behaviour script (for velocity based animation)
                //data.animatable.m_currentTime += deltaTime;

                // PLEEPLOG_DEBUG("Animating " + data.animatable.m_currentAnimation + " on armature " + data.renderable.armature.m_name + " with bones: " + std::to_string(data.renderable.armature.m_bones.size()));

                // Need to compose transform recursively starting from root bone...?
                calculate_bones_transform(
                    data.renderable.armature,
                    data.animatable.animations[data.animatable.m_currentAnimation],
                    0,  // boneIndex 0 should be root of armature
                    glm::mat4(1.0f),//data.renderable.armature.m_relativeTransform,//
                    data.animatable.m_currentTime
                );

                // PLEEPLOG_DEBUG("Done animation");
            }
        }

        // recursive tree crawl to set all bones
        void calculate_bones_transform(Armature& armature, std::shared_ptr<const AnimationSkeletal> animation, const unsigned int boneIndex, glm::mat4 parentTransform, const double elapsedTime)
        {
            if (animation->m_duration <= 0) return;
            const double currentTick = elapsedTime * animation->m_frequency;
            const double progress = fmod(currentTick / animation->m_duration, 1.0);
            
            // for non-animated bones use relative transform
            glm::mat4 interpolatedTransform = armature.m_bones[boneIndex].m_relativeTransform;
            
            // for animated bones OVERRIDE with keyframe position using elapsed time
            if (animation->m_posKeyframes.count(boneIndex) || 
                animation->m_rotKeyframes.count(boneIndex) || 
                animation->m_sclKeyframes.count(boneIndex))
            {
                glm::mat4 translate(1.0f);
                glm::mat4 rotate(1.0f);
                glm::mat4 scale(1.0f);

                if (animation->m_posKeyframes.count(boneIndex))
                {
                    size_t posFrame = static_cast<size_t>(progress * animation->m_posKeyframes.at(boneIndex).size());
                    translate = glm::translate(glm::mat4(1.0f), animation->m_posKeyframes.at(boneIndex).at(posFrame).position);
                }

                if (animation->m_rotKeyframes.count(boneIndex))
                {
                    size_t rotFrame = static_cast<size_t>(progress * animation->m_rotKeyframes.at(boneIndex).size());
                    //PLEEPLOG_DEBUG("Animation frame: " + std::to_string(rotFrame));
                    //const glm::quat key = animation->m_rotKeyframes.at(boneIndex).at(rotFrame).orientation;
                    //PLEEPLOG_DEBUG(std::to_string(key.x) + ", " + std::to_string(key.y) + ", " + std::to_string(key.z) + ", " + std::to_string(key.w));
                    rotate = glm::toMat4(glm::normalize(animation->m_rotKeyframes.at(boneIndex).at(rotFrame).orientation));
                }

                if (animation->m_sclKeyframes.count(boneIndex))
                {
                    size_t sclFrame = static_cast<size_t>(progress * animation->m_sclKeyframes.at(boneIndex).size());
                    scale = glm::scale(glm::mat4(1.0f), animation->m_sclKeyframes.at(boneIndex).at(sclFrame).scale);
                }
            
                //PLEEPLOG_DEBUG("Bone " + armature.m_bones[boneIndex].m_name + " is animated");
                interpolatedTransform = translate * rotate * scale;
            }

            // calculate transform for this "node"
            const glm::mat4 globalTransform = parentTransform * interpolatedTransform;

            // set bone's cached transform using parent & keyframe (and inverse bind)
            armature.m_bones[boneIndex].m_localTransform = globalTransform * armature.m_bones[boneIndex].m_bindTransform;

            // pass onto children
            for (unsigned int childId : armature.m_bones[boneIndex].m_childIds)
            {
                calculate_bones_transform(armature, animation, childId, globalTransform, elapsedTime);
            }
        }

        void clear_bones_transform(std::vector<Bone>& bones)
        {
            for (Bone& bone : bones)
            {
                bone.m_localTransform = glm::mat4(1.0f);
            }
        }
        
        void clear()
        {
            m_animationPackets.clear();
        }

        void submit(AnimationPacket data)
        {
            m_animationPackets.push_back(data);
        }

    private:
        std::vector<AnimationPacket> m_animationPackets;
    };
}

#endif // ANIMATION_RELAY_H