cmake_minimum_required(VERSION 3.19.1)

project(Psychri-la VERSION 0.1)

# use known executable name to be link to by sub-libraries
add_executable(${PROJECT_NAME} source/main.cpp)

# link to internal source and libraries
add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PRIVATE source)

# link to external library (sub_module)
