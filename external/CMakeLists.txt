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

# "immediate" building
# extra glfw build options (can be set on cmake configure with -D)
#set(GLFW_BUILD_DOCS OFF CACHE BOOL [FORCE])
#set(GLFW_BUILD_TESTS OFF CACHE BOOL [FORCE])
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL [FORCE])
#add_subdirectory(glfw)
#target_link_libraries(${PROJECT_NAME} glfw)

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glfw/build")
    message(FATAL_ERROR "GLFW has not been precompiled into a library. Please manually run glfw_precompile_library.sh")
    add_custom_command(COMMAND ../cmake_scripts/glfw_precompile_library.sh)
endif()

# if we precompile glfw into a static library can we link there directly
target_link_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/glfw/build/src/${CONFIGURATION})
target_link_libraries(${ENGINE_NAME} glfw3)

target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/glfw/include)


# GLAD
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glad/src/glad.c")
    message(FATAL_ERROR "The GLAD library is not available! Please see external/README.md to regenerate it.")
endif()

# include cpp headers. NOTE: glad also has source files accounted for in ./source_files.cmake
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/glad/include)

# GLM
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glm/CMakeLists.txt")
    message(FATAL_ERROR "The GLM submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please manually update submodules.")
endif()

# glm is header only?
#add_subdirectory(glm)
#target_link_libraries(${PROJECT_NAME} glm)
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/glm)

# STB_IMAGE
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stb_image)

# ASSIMP
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/assimp/CMakeLists.txt")
message(FATAL_ERROR "The assimp submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please manually update submodules.")
endif()

# "immediate" build
# extra assimp build options
#set(BUILD_SHARED_LIBS OFF CACHE BOOL [FORCE])
#set(ASSIMP_BUILD_ZLIB ON CACHE BOOL [FORCE])
#add_subdirectory(assimp)
#target_link_libraries(${PROJECT_NAME} assimp)

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/assimp/build")
    message(FATAL_ERROR "ASSIMP has not been precompiled into a library. Please manually run assimp_precompile_library.sh")
endif()

# if we precompile assimp into a dll or static lib can we link there directly
target_link_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/assimp/build/lib/${CONFIGURATION})
# Assimp uses a debug postfix "d"
target_link_libraries(${ENGINE_NAME}
    debug assimp-vc143-mtd
    optimized assimp-vc143-mt
)
# include cmake generated header
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/assimp/build/include)
# include cpp headers
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/assimp/include)

# also required to build and link zlib if assimp was built into static lib
target_link_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/assimp/build/contrib/zlib/${CONFIGURATION})
# Zlib will also use a debug postfix "d"
target_link_libraries(${ENGINE_NAME}
    debug zlibstaticd
    optimized zlibstatic
)


# SPDLOG
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/spdlog/CMakeLists.txt")
message(FATAL_ERROR "The spdlog submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please manually update submodules.")
endif()

# spdlog is header only?
# include cpp headers
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/spdlog/include)


# IMGUI
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/imgui/imgui.h")
message(FATAL_ERROR "The imgui submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please manually update submodules.")
endif()

# no CMakeLists.txt, source is added in source_files.cmake
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui)
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui/backends)


# ASIO
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/asio/include/asio.hpp")
message(FATAL_ERROR "The ASIO library is not available! Please see external/README.md and manually download it")
endif()

# header only
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/asio/include)
