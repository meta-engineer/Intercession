#!/bin/bash
# run each component script in order for full fresh build

# push working directory
START_PWD=$(pwd)

if [[ $(basename $START_PWD) == "Intercession" ]]
then
    true
elif [[ $(basename $START_PWD) == "cmake_scripts" ]]
then
    cd ..
else
    echo "-- Please run script from directory cmake_scripts/ or project root Intercession/"
    exit
fi

git submodule update --init --recursive

./cmake_scripts/glfw_precompile_library.sh
./cmake_scripts/assimp_precompile_library.sh
./cmake_scripts/fresh_cmake_configure.sh
./cmake_scripts/release_build_cmake.sh

if [ -f "./build/Release/INTERCESSION.exe" ]
then
    echo ""
    echo "-- Build success!"
    echo "-- Release executable: Intercession/build/Release/INTERCESSION.exe"
    echo "-- Run debug_build_cmake.sh to build Debug config as well."
    echo "-- Run package.sh to build program installer."
    echo ""
    echo "-- Start executable with \"INTERCESSION.exe - Release\" shortcut."
else
    echo ""
    echo "-- Build Failure."
    echo "-- Please check script output"
fi

# pop working directory
cd ${START_PWD}