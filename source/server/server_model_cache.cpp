#include "rendering/model_cache.h"

#include "rendering/model_manager_faux.h"

namespace pleep
{
namespace ModelCache
{
    // Make faux ModelManager to avoid any gpu api calls
    // Cannot log from constructor becuase logger has not initialized until main
    std::unique_ptr<ModelManager> g_modelManager = std::make_unique<ModelManagerFaux>();
}
}
