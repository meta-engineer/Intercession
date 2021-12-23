cmake_minimum_required(VERSION 3.19.1)

project(EIDOLON VERSION 0.1)

# use known executable name to be linked to by sub-libraries
# update source)files.cmake with everything to compile
include(source/source_files.cmake)
include(external/source_files.cmake)
add_executable(${PROJECT_NAME} ${ALL_SOURCE_FILES} ${ALL_EXTERNAL_SOURCE_FILES} config/AppIcon.rc)

# global compiler warning options
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  # /W4 for warnings, /WX for warnings as errors
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
  # -Wall -Wextra -Wpedantic -Werror
endif()

# link to internal source and libraries
add_subdirectory(source)

# define config after variables from source tree have been set
configure_file(config/build_config.h.in source/build_config.h)
# config gets generated to binary directory's source
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR}/source)


# link to external libraries (submodule)
# external will download git submodules before including
add_subdirectory(external)


# Properties
# disable the console window (will have to have x-platform checks for this)
#set_target_properties(${PROJECT_NAME} PROPERTIES
#    LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS"
#)


# cmake install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/source/build_config.h" DESTINATION include)
# packaging includes & options
add_subdirectory(package)
