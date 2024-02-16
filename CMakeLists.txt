cmake_minimum_required(VERSION 3.19.1)

project(INTERCESSION VERSION 0.8.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENGINE_NAME ${PROJECT_NAME}_ENGINE)
set(SERVER_NAME ${PROJECT_NAME}_SERVER)
set(CLIENT_NAME ${PROJECT_NAME}_CLIENT)
set(DISPATCHER_NAME ${PROJECT_NAME}_DISPATCHER)

# use known executable name to be linked to by sub-libraries
# update source_files.cmake with everything to compile
include(source/source_files.cmake)
include(external/source_files.cmake)

# TODO: common engine code should probably be compiled into a shared library
add_library(${ENGINE_NAME} ${COMMON_SOURCE_FILES} ${EXTERNAL_SOURCE_FILES})
# and client and server link to it instead of compiling the same common/external sources
add_executable(${SERVER_NAME} ${SERVER_SOURCE_FILES} config/_server_icon.rc)
target_link_libraries(${SERVER_NAME} ${ENGINE_NAME})
add_executable(${CLIENT_NAME} ${CLIENT_SOURCE_FILES} config/_client_icon.rc)
target_link_libraries(${CLIENT_NAME} ${ENGINE_NAME})

# global compiler warning options
if(MSVC)
  target_compile_options(${ENGINE_NAME} PRIVATE /W4)
  target_compile_options(${SERVER_NAME} PRIVATE /W4)
  target_compile_options(${CLIENT_NAME} PRIVATE /W4)
  # /W4 for warnings, /WX for warnings as errors
else()
  target_compile_options(${ENGINE_NAME} PRIVATE -Wall)
  target_compile_options(${SERVER_NAME} PRIVATE -Wall)
  target_compile_options(${CLIENT_NAME} PRIVATE -Wall)
  # -Wall -Wextra -Wpedantic -Werror
endif()


# link to internal source and libraries
add_subdirectory(source)

# define config after variables from source tree have been set
configure_file(config/build_config.h.in source/build_config.h)
# config gets generated to binary directory's source for ALL targets
target_include_directories(${ENGINE_NAME} PUBLIC ${PROJECT_BINARY_DIR}/source)
target_include_directories(${SERVER_NAME} PUBLIC ${PROJECT_BINARY_DIR}/source)
target_include_directories(${CLIENT_NAME} PUBLIC ${PROJECT_BINARY_DIR}/source)


# link to external libraries (submodule)
# external will download git submodules before including
add_subdirectory(external)


# Properties
# disable the console window (will have to have x-platform checks for this)
#set_target_properties(${PROJECT_NAME} PROPERTIES
#    LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS"
#)


# cmake install
install(TARGETS ${SERVER_NAME} DESTINATION bin)
install(TARGETS ${CLIENT_NAME} DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/source/build_config.h" DESTINATION include)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/resources/" DESTINATION "resources")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/source/shaders/" DESTINATION "source/shaders")
# packaging includes & options
add_subdirectory(packaging)
