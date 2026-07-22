#!/bin/bash
set -e

# Build only testdialog.c, reusing SDL3 built by CMake.

BUILD_DIR="$(cd "$(dirname "$0")/build" && pwd)"
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"

SDL_SRC="$BUILD_DIR/_deps/sdl-src"
SDL_BUILD="$BUILD_DIR/_deps/sdl-build"

SDL_INC="$SDL_SRC/include"
SDL_CONFIG_INC="$SDL_BUILD/include-config-"
# the config dir has a unique suffix; find it
SDL_CONFIG_DIR=$(echo "$SDL_BUILD"/include-config-*)

gcc -std=c99 -Wall -Wextra -pedantic \
    -I "$SDL_INC"                            \
    -I "$SDL_CONFIG_DIR"                     \
    -I "$SDL_BUILD/include"                  \
    "$SRC_DIR/testdialog.c"                  \
    -L "$SDL_BUILD"                          \
    -Wl,-rpath,"$SDL_BUILD"                 \
    -l SDL3                                  \
    -o "$SRC_DIR/testdialog"                 \

echo "→ Built testdialog"
