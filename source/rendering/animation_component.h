#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

//#include "intercession_pch.h"
#include <map>
#include <string>

#include "rendering/animation_skeletal.h"

namespace pleep
{
    class AnimationComponent
    {
        // TODO: should all animations be stored in one component?
        std::unordered_map<std::string, AnimationSkeletal> animations;
    };
}

#endif // ANIMATION_COMPONENT_H