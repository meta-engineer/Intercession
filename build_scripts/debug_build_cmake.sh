#!/bin/bash
# config and build
# try running fresh_cmake_configure.sh to troubleshoot

# pwd is root
cd D:/Projects/Programs/Intercession

cmake . -B build
cmake --build build