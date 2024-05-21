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

                // Need to compose transform recursively starting from root bone...?
                calculate_bones_transform(
                    data.renderable.armature.m_bones,
                    data.animatable.animations[data.animatable.m_currentAnimation],
                    0,
                    glm::mat4(1.0f),
                    data.animatable.m_currentTime
                );

                //PLEEPLOG_DEBUG("Animating " + data.animatable.m_currentAnimation + " on armature " + data.renderable.armature.m_name + " with bones: " + std::to_string(data.renderable.armature.m_bones.size()));
            }
        }

        // recursive tree crawl to set all
        void calculate_bones_transform(std::vector<Bone>& bones, std::shared_ptr<const AnimationSkeletal> animation, const unsigned int boneIndex, glm::mat4 parentTransform, const double elapsedTime)
        {
            size_t currentFrame = static_cast<size_t>((elapsedTime * animation->m_frequency) / animation->m_duration * animation->m_posKeyframes.at(boneIndex).size());
            currentFrame = currentFrame % animation->m_posKeyframes.at(boneIndex).size();

            // derive keyframe position for this bone using elapsed time
            glm::mat4 interpolatedTransform = 
                glm::translate(glm::mat4(1.0f), animation->m_posKeyframes.at(boneIndex).at(currentFrame).position) *
                glm::toMat4(glm::normalize(animation->m_rotKeyframes.at(boneIndex).at(currentFrame).orientation)) *
                glm::scale(glm::mat4(1.0f), animation->m_sclKeyframes.at(boneIndex).at(currentFrame).scale);

            // calculate transform for this "node"
            glm::mat4 globalTransform = parentTransform * interpolatedTransform;

            // set bone's cached transform using parent & keyframe (and inverse bind)
            bones[boneIndex].m_localTransform = globalTransform * bones[boneIndex].m_bindTransform;

            // pass onto children
            for (unsigned int childId : bones[boneIndex].m_childIds)
            {
                calculate_bones_transform(bones, animation, childId, globalTransform, elapsedTime);
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