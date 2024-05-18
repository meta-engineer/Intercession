#ifndef ANIMATION_PACKET_H
#define ANIMATION_PACKET_H

//#include "intercession_pch.h"
#include "rendering/renderable_component.h"
#include "rendering/animation_component.h"

namespace pleep
{
    struct AnimationPacket
    {
        RenderableComponent& renderable;
        AnimationComponent& animatable;    
    };
}

#endif // ANIMATION_PACKET_H