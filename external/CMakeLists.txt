# Manage inclusion of external sources

# download submodule libraries before including
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if (GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if (NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules.")
        endif()
    endif()
endif()

# TODO: set variable for all external libraries, and ust one target_link_libraries

# GLFW
# check, build, link, and include header
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    message(FATAL_ERROR "The GLFW submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please manually update submodules.")
endif()

add_subdirectory(glfw)
target_link_libraries(${PROJECT_NAME} glfw)
target_include_directories(${PROJECT_NAME} PUBLIC glfw/include)

# extra glfw build options (can be set on cmake configure with -D)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)


# GLAD
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glad/src/glad.c")
    message(FATAL_ERROR "The GLAD sis not available! Please see external/README.md to regenerate it.")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC glad/include)