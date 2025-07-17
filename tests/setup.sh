#!/bin/bash

if [ ! -d .venv ]; then
  python -m venv .venv
fi
echo "source .venv/bin/activate"