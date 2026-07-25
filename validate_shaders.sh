#!/bin/bash
# Validate all GLSL shaders
set -e
cd "$(dirname "$0")"
python3 dev/z1.py validate-shaders "$@"
