#!/bin/bash

rm -rf build/
mkdir -p build
cd build
CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=Debug -GNinja ..