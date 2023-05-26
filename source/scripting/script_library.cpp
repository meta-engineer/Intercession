#include "scripting/script_library.h"

// registered scripts
#include "scripting/biped_scripts.h"
#include "scripting/fly_control_scripts.h"
#include "scripting/lakitu_scripts.h"
#include "scripting/oscillator_scripts.h"

namespace pleep
{
    std::shared_ptr<I_ScriptDrivetrain> ScriptLibrary::fetch_script(ScriptType sType)
    {
        std::shared_ptr<I_ScriptDrivetrain> newDrivetrain = nullptr;
        switch(sType)
        {
            case ScriptType::fly_control:
                newDrivetrain = std::make_shared<FlyControlScripts>();
                break;
            case ScriptType::lakitu:
                newDrivetrain = std::make_shared<LakituScripts>();
                break;
            case ScriptType::biped_control:
                newDrivetrain = std::make_shared<BipedScripts>();
                break;
            case ScriptType::oscillator:
                newDrivetrain = std::make_shared<OscillatorScripts>();
                break;
            default:
                //PLEEPLOG_WARN("Found unknown script enum value: " + std::to_string(static_cast<uint16_t>(sType)));
                break;
        }
        newDrivetrain->m_libraryType = sType;
        return newDrivetrain;
    }
}