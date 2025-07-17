#!/bin/bash

if [ -d writing-a-c-compiler-tests ]; then
  git clone git@github.com:nlsandler/writing-a-c-compiler-tests.git
else
  cd writing-a-c-compiler-tests
  git pull
fi