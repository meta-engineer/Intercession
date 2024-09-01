#!/bin/bash
# fresh configure & build of assimp static lib
# NOTE: a newly fetched assimp may be an updated version and the
# library name may have changed accordingly (currently assimp-vc143-mt.lib)
# after running this script check your version (external/assimp/build/lib/Debug)
# and match the name in external/CmakeLists.txt
# Can we specify the gitmoduile use a single version?

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

echo "-- Building ASSIMP into static library"
if [ -d "./external/assimp/build" ]
then
    echo "-- Clearing old assimp/build"
    rm ./external/assimp/build -r
fi

# compile as static, leave debug postfixes alone (external/CMakeLists.txt assumes this)
cmake ./external/assimp/ -B ./external/assimp/build -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ZLIB=ON

# debug build
cmake --build ./external/assimp/build

#release build
cmake --build ./external/assimp/build --config Release


# pop working directory
cd ${START_PWD}