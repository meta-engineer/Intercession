#!/bin/bash
# fresh configure

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

echo "-- Configuring Intercession build files"
if [ -d "./build" ]
then
    echo "-- Clearing old /build"
    rm ./build -r
fi

cmake . -B build -G "Visual Studio 17 2022"


# pop working directory
cd ${START_PWD}