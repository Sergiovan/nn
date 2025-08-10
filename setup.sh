#!/bin/bash

mkdir -p build
cd build

if [ ! -v CC ]; then
  CC=clang++
fi

if [ ! -v CXX ]; then
  CXX=clang++
fi

(
# Debug
mkdir -p debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -GNinja ../..
)

(
# Release
mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ../..
)