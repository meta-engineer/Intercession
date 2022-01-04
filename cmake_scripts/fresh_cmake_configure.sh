#!/bin/bash
# fresh configure

# pwd is root
cd D:/Projects/Programs/Intercession

if [ -d "./build" ]
then
    echo "-- Clearing old /build"
    rm ./build -r
fi

cmake . -B build