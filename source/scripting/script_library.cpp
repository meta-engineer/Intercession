#include "scripting/script_library.h"

namespace pleep
{
    // Create static library instance immediately
    // we must call constructor ourselves (it is protected)
    // but we pass ownership to unique_ptr, so no delete needed
    std::unique_ptr<ScriptLibrary> ScriptLibrary::m_singleton(new ScriptLibrary);
}