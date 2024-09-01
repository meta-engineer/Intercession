#!/bin/bash
# package, and move package to root

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

echo "-- Packaging release build into installer"
# release_build_cmake.sh must have been run
if [ ! -d "./build/Release" ]
then
    echo "Release build does not exist. Please manually run release_build_cmake.sh"
    exit 1
fi

cd build
cpack
cd ..

if [ ! -f "./build/INTERCESSION-installer.exe" ]
then
    echo "Could not find installer executable, check cpack"
    exit 1
fi

echo "****** Installer found! Moving to $PWD"
mv build/INTERCESSION-installer.exe INTERCESSION-installer.exe

# pop working directory
cd ${START_PWD}