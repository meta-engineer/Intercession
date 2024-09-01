#!/bin/bash
# fresh configure & build of glfw static lib

# push working directory
START_PWD=$(pwd)

if [[ $(basename $START_PWD) == "Intercession" ]]
then
    true
elif [[ $(basename $START_PWD) == "cmake_scripts" ]]
then
    cd ..
elif [[ $(basename $START_PWD) == "external" ]]
then
    cd ..
else
    echo "-- Please run script from directory cmake_scripts/ or project root Intercession/"
    exit
fi

echo "-- Building GLFW into static library"
if [ -d "./external/glfw/build" ]
then
    echo "-- Clearing old glfw/build"
    rm ./external/glfw/build -r
fi

# compile as static, leave debug postfixes alone (external/CMakeLists.txt assumes this)
cmake ./external/glfw/ -B ./external/glfw/build -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF

# debug build
cmake --build ./external/glfw/build

#release build
cmake --build ./external/glfw/build --config Release


# pop working directory
cd ${START_PWD}