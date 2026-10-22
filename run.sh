#!/usr/bin/env bash
# Compile and run from Ubuntu/WSL using the bundled GLAD source.
set -euo pipefail
cd -- "$(dirname -- "$0")"
mkdir -p build
g++ -std=c++17 -Wall -Wextra main.cpp external/glad/src/gl.c \
    -Iexternal/glad/include -o build/space_shooter -lglfw
if [[ "${1:-}" != "--build-only" ]]; then
    exec ./build/space_shooter
fi
