#!/bin/bash
# Generate macOS Makefiles via premake5
set -e
cd "$(dirname "$0")"
python3 dev/z1.py generate "$@"
