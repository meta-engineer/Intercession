#include "view_manager.h"

#include "GLFW/glfw3.h"

namespace view_manager
{
    int get_context_id()
    {
        return 4;
    }

    int get_glfw_version_major()
    {
        int major;
        int minor;
        int rev;
        glfwGetVersion(&major, &minor, &rev);
        return major;
    }

    bool init_window()
    {
        return true;
    }

    bool run_window()
    {
        return true;
    }
}
