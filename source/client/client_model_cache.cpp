#include "rendering/model_cache.h"

namespace pleep
{
namespace ModelCache
{
    // Cannot log from constructor becuase logger has not initialized until main
    std::unique_ptr<ModelManager> g_modelManager = std::make_unique<ModelManager>();
}
}
