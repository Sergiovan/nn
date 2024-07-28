#!/bin/bash

mkdir -p build
cd build
CXX=clang++ CC=clang cmake -GNinja ..