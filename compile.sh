#!/bin/bash
# Compile z1engine on macOS
set -e
cd "$(dirname "$0")"
export CFLAGS="-w ${CFLAGS}"
export CXXFLAGS="-w ${CXXFLAGS}"
python3 dev/z1.py compile "$@"
