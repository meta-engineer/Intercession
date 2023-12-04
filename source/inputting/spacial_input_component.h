#ifndef SPACIAL_INPUT_COMPONENT_H
#define SPACIAL_INPUT_COMPONENT_H

//#include "intercession_pch.h"
#include <bitset>
#include <array>

namespace pleep
{
    // all inputs SpacialInputComponent can receive
    // actions should be ambiguous of source input device
    // should implicitly convert to int
    enum SpacialActions
    {
        // absolute movement?

        // relative movement
        moveParallel,   // parallel to forward vector
        moveHorizontal,
        moveVertical,

        // relative rotation
        rotatePitch,
        rotateYaw,
        rotateRoll, // + is cw

        // cast a ray out from entity
        raycastX,
        raycastY,
        raycastZ,

        // specify a world-space coordinate to target
        targetX,
        targetY,
        targetZ,

        // generic
        // bit represets active or not
        // value of 1.0 means edge, value of 0.0 means sustained
        action0,
        action1,
        action2,
        action3,
        action4,
        action5,

        // etc...

        spacial_actions_count
    };

    // Interprets input as control of an object in 3d space
    struct SpacialInputComponent
    {
        // TODO: Permissions, should all entities with this component be receiving data?

        // set of SpacialActions
        std::bitset<SpacialActions::spacial_actions_count> actions;
        // matching size array of associated values for actions
        std::array<double, SpacialActions::spacial_actions_count> actionVals{};

        // convenience method for returning to default values
        inline void clear()
        {
            actions.reset();
            // if actionVals are ALWAYS set with actions then we dont need to set 0's
        }

        // move "hot" states to "ground" value
        inline void flush()
        {
            // rotations controlled by mouse need to be reset (there is no mousemove "release")
            actions.set(SpacialActions::rotatePitch, false);
            actions.set(SpacialActions::rotateYaw,   false);
            actions.set(SpacialActions::rotateRoll,  false);
        }

        inline void set(SpacialActions action, bool state, double value = 0.0f)
        {
            actions.set(action, state);
            actionVals.at(action) = value;
        }
    };
}

#endif // SPACIAL_INPUT_COMPONENT_H