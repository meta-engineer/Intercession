#include "behaviors/behaviors_library.h"

// registered behaviors
#include "behaviors/biped_behaviors.h"
#include "behaviors/fly_control_behaviors.h"
#include "behaviors/lakitu_behaviors.h"
#include "behaviors/oscillator_behaviors.h"
#include "behaviors/projectile_behaviors.h"

namespace pleep
{
    std::shared_ptr<I_BehaviorsDrivetrain> BehaviorsLibrary::fetch_behaviors(BehaviorsType sType)
    {
        std::shared_ptr<I_BehaviorsDrivetrain> newDrivetrain = nullptr;
        switch(sType)
        {
            case BehaviorsType::fly_control:
                newDrivetrain = std::make_shared<FlyControlBehaviors>();
                break;
            case BehaviorsType::lakitu:
                newDrivetrain = std::make_shared<LakituBehaviors>();
                break;
            case BehaviorsType::biped_control:
                newDrivetrain = std::make_shared<BipedBehaviors>();
                break;
            case BehaviorsType::oscillator:
                newDrivetrain = std::make_shared<OscillatorBehaviors>();
                break;
            case BehaviorsType::projectile:
                newDrivetrain = std::make_shared<ProjectileBehaviors>();
                break;
            default:
                PLEEPLOG_WARN("Found unknown behaviors enum value: " + std::to_string(static_cast<uint16_t>(sType)));
                return nullptr;
        }

        newDrivetrain->m_libraryType = sType;
        return newDrivetrain;
    }
}