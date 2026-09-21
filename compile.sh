#!/bin/bash
# Compile z1engine on macOS
set -e
cd "$(dirname "$0")"
python3 dev/z1.py compile "$@"
