cmake_minimum_required(VERSION 3.19.1)

project(EIDOLON VERSION 0.1)

# use known executable name to be link to by sub-libraries
add_executable(${PROJECT_NAME} source/main.cpp)

configure_file(source/build_config.h.in source/build_config.h)
# config gets generated to binary directory's source
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR}/source)

# link to internal source and libraries
add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PRIVATE source)

# link to external libraries (submodule)
# external will download git submodules before including
add_subdirectory(external)
# include_directories managed for each libraries structure
