#!/usr/bin/env zsh

HERE=${0:a:h}

if [[ $ZSH_EVAL_CONTEXT == 'toplevel' ]]; then
    echo "Please source this file"
    exit 1
fi


if [ ! -d .venv ]; then
  python -m venv $HERE/.venv
fi

source $HERE/.venv/bin/activate
python -m pip install --upgrade pip
pip install -r $HERE/requirements.txt