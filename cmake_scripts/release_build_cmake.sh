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

if [ -d "./build" ]
then
    echo "-- Building Intercession in Release mode"
    cmake --build build --config Release
else
    echo "-- Build files do not exist, run fresh_cmake_configure.sh first"
fi


# pop working directory
cd ${START_PWD}