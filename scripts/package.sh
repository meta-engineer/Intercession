#!/bin/bash
# package, and move package to root

cd D:/Projects/Programs/OpenGL_2021
# release_build_cmake.sh must have been run
if [ ! -d "./build/Release" ]
then
    echo "Release build does not exist. Please manually run release_build_cmake.sh"
    exit 1
fi

cmake --build build --config Release
cd build
cpack
cd ..

mv build/EIDOLON-installer.exe EIDOLON-installer.exe
echo "****** Moved installer to $PWD"
