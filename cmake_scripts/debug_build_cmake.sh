#!/bin/bash
# config and build
# try running fresh_cmake_configure.sh to troubleshoot

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

echo "-- Building Intercession in Debug mode"
#cmake . -B build
cmake --build build


# pop working directory
cd ${START_PWD}