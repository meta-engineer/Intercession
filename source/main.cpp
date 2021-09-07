// Copyright Pleep inc. 2021

// std libraries
#include <iostream>

// external
#include "GLFW/glfw3.h"

// internal
#include "build_config.h"
#ifdef USE_VIEW_MANAGER
#include "view_manager/view_manager.h"
#endif

int main(int argc, char** argv) {
    std::cout << argv[0] << " Version " << BUILD_VERSION_MAJOR << "." << BUILD_VERSION_MINOR << std::endl;

    std::cout << "We're gaming!" << std::endl;
    std::cout << glfwGetVersionString() << std::endl;
    
#ifdef USE_VIEW_MANAGER
    std::cout << view_manager::get_context_id() << std::endl;
    std::cout << view_manager::get_glfw_version_major() << std::endl;
#endif
}
