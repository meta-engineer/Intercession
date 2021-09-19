#!/bin/bash
# fresh configure

# pwd is root
cd D:/Projects/Programs/OpenGL_2021

if [ -d "./build" ]
then
    echo "-- Clearing old /build"
    rm ./build -r
fi

cmake . -B build