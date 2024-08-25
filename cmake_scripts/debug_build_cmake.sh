#!/bin/bash
# config and build
# pass specified targets to build as each commandline argument, or none for all targets
# try running fresh_cmake_configure.sh to troubleshoot

# push working directory
START_PWD=$(pwd)

if [[ $(basename $START_PWD) == "Intercession_dev" ]]
then
    true
elif [[ $(basename $START_PWD) == "cmake_scripts" ]]
then
    cd ..
else
    echo "-- Please run script from directory cmake_scripts/ or project root Intercession_dev/"
    exit
fi

# concat select targets as cmake cmd options
TARGETS=""
TARGET_OPTIONS=""
for TARGET in "$@"
do
    TARGETS+=" $TARGET"
    TARGET_OPTIONS+=" --target $TARGET"
done

if [ -d "./build" ]
then
    echo "-- Building Intercession in Debug mode"
    if [[ $# -ne 0 ]]
    then
        echo "-- Only targets:$TARGETS"
    fi

    cmake --build build $TARGET_OPTIONS
else
    echo "-- Build files do not exist, run fresh_cmake_configure.sh first"
fi


# pop working directory
cd ${START_PWD}