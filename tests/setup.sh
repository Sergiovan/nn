#!/usr/bin/env zsh

if [[ $ZSH_EVAL_CONTEXT == 'toplevel' ]]; then
    echo "Please source this file"
    exit 1
fi


if [ ! -d .venv ]; then
  python -m venv .venv
fi

source .venv/bin/activate
python -m pip install --upgrade pip
pip install -r requirements.txt