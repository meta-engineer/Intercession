#!/bin/bash
# package, and move package to root

cd D:/Projects/Programs/Intercession
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
