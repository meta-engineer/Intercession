#ifndef SPACIAL_INPUT_BUFFER_H
#define SPACIAL_INPUT_BUFFER_H

//#include "intercession_pch.h"
#include <bitset>
#include <array>

namespace pleep
{
    // represents  commands a controller can recieve for moving in space
    // actions should be ambiguous of source input device
    // should implicitly convert to int
    enum SpacialActions
    {
        // absolute movement?

        // relative movement
        moveForward,
        moveBackward,
        moveLeft,
        moveRight,
        moveUp,
        moveDown,

        // absolute rotation
        rotatePitchUp,
        rotatePitchDown,
        rotateYawLeft,
        rotateYawRight,
        rotateRollCw,
        rotateRollCcw,

        // generic
        action0,
        action1,
        action2,
        action3,
        action4,
        action5,

        // etc...

        count
    };

    // this is an abstraction of all information needed for an entity from an input device
    // actions bit is set if related input is active
    // actionVal is set if related action is set, 
    struct SpacialInputBuffer
    {
        // set of SpacialActions
        std::bitset<SpacialActions::count> actions;
        // matching size array of associated values for actions
        std::array<float, SpacialActions::count> actionVals{};

        // convenience method for returning to default values
        void clear()
        {
            actions.reset();
            // if actionVals are ALWAYS set with actions then we dont need to set 0's
        }

        // move "hot" states to "ground" value
        void resolve()
        {
            // rotations controlled by mouse need to be reset (there is no mousemove "release")
            actions.set(SpacialActions::rotatePitchUp,   false);
            actions.set(SpacialActions::rotatePitchDown, false);
            actions.set(SpacialActions::rotateYawLeft,   false);
            actions.set(SpacialActions::rotateYawRight,  false);
            actions.set(SpacialActions::rotateRollCw,    false);
            actions.set(SpacialActions::rotateRollCcw,   false);
        }

        void set(SpacialActions action, bool state, float value = 0.0f)
        {
            actions.set(action, state);
            actionVals.at(action) = value;
        }
    };
}

#endif // SPACIAL_INPUT_BUFFER_H