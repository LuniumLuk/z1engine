#!/bin/bash
# Run the z1engine game
set -e
cd "$(dirname "$0")"
exec ./engine/bin/Hybrid/game --game --scene=scene/demo_scene
