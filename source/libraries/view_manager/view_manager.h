
// container for glfw view

#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

namespace view_manager
{
    int get_context_id();
    int get_glfw_version_major();
    bool init_window();
    bool run_window();
}

#endif