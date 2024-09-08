#!/bin/bash

mkdir -p build
cd build

(
# Debug
mkdir -p debug
cd debug
CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -GNinja ../..
)

(
# Release
mkdir -p release
cd release
CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=Release -GNinja ../..
)