#ifndef SCRIPT_LIBRARY_H
#define SCRIPT_LIBRARY_H

//#include "intercession_pch.h"
#include <memory>
#include <string>
#include <typeinfo>

#include "logging/pleep_log.h"

namespace pleep
{
    // forward declare I_ScriptDrivetrain so it can import ScriptLibrary::ScriptType
    class I_ScriptDrivetrain;

    // A static (global) access point to coordinate memory for script objects
    //  (as multiple entities could want to use the same script)
    // Scripts are for functionality not specific enough to justify a dedicated dynamo
    // or for functionality which required components cannot be strictly defined
    // However, we still want the efficiency afforded by funneling components to a single relay.
    // ScriptComponents shouldn't create their own script object, but fetch it from a ScriptLibrary
    // which can manage their allocation in lieu of a dynamo.
    // In order to ensure all users have the same instance this will be a static singleton.
    class ScriptLibrary
    {
    protected:
        // Cannot log from this constructor because it is invoked before logger init in main
        ScriptLibrary() = default;
    public:
        ~ScriptLibrary() = default;
        // copy constructor
        ScriptLibrary(const ScriptLibrary&) = delete;

        // list all available scripts as enum for serialization
        // Each I_ScriptDrivetrain subclass could list their associated ScriptType?
        //     or Script library could be kept closed
        //     but then it would need to be able to scan a script type for its enum
        //     Alternatively Script library could assign a ScriptType value to a member of the script (like model library sets the source filename)
        enum class ScriptType
        {
            none,
            fly_control,            // FlyControlScripts
            biped_control,          // BipedScripts
            oscillator,             // OscillatorScripts
            count
        };

        static std::shared_ptr<I_ScriptDrivetrain> fetch_script(ScriptType sType);
    };
}

#endif // SCRIPT_LIBRARY_H