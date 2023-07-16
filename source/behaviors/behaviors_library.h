#ifndef BEHAVIORS_LIBRARY_H
#define BEHAVIORS_LIBRARY_H

//#include "intercession_pch.h"
#include <memory>
#include <string>
#include <typeinfo>

#include "logging/pleep_log.h"

namespace pleep
{
    // forward declare I_BehaviorsDrivetrain so it can import BehaviorsLibrary::BehaviorsType
    class I_BehaviorsDrivetrain;

    // A static (global) access point to coordinate memory for behaviors objects
    //  (as multiple entities could want to use the same behaviors)
    // Behaviors are for functionality not specific enough to justify a dedicated dynamo
    // or for functionality which required components cannot be strictly defined
    // However, we still want the efficiency afforded by funneling components to a single relay.
    // BehaviorsComponents shouldn't create their own behaviors object, but fetch it from a BehaviorsLibrary
    // which can manage their allocation in lieu of a dynamo.
    // In order to ensure all users have the same instance this will be a static singleton.
    class BehaviorsLibrary
    {
    protected:
        // Cannot log from this constructor because it is invoked before logger init in main
        BehaviorsLibrary() = default;
    public:
        ~BehaviorsLibrary() = default;
        // copy constructor
        BehaviorsLibrary(const BehaviorsLibrary&) = delete;

        // list all available behaviors as enum for serialization
        // Each I_BehaviorsDrivetrain subclass could list their associated BehaviorsType?
        //     or Behaviors library could be kept closed
        //     but then it would need to be able to scan a behaviors type for its enum
        //     Alternatively Behaviors library could assign a BehaviorsType value to a member of the behaviors (like model library sets the source filename)
        enum class BehaviorsType
        {
            none,
            fly_control,            // FlyControlBehaviors
            lakitu,                 // LakituBehaviors
            biped_control,          // BipedBehaviors
            oscillator,             // OscillatorBehaviors
            count
        };

        static std::shared_ptr<I_BehaviorsDrivetrain> fetch_behaviors(BehaviorsType sType);
    };
}

#endif // BEHAVIORS_LIBRARY_H