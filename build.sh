#!/bin/bash
set -e

BUILD_TYPE="Debug"

if [ "$1" == "-c" ]||[ "$1" == "-clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
    rm -rf .cache
    shift
fi

if [ "$1" == "-r" ]||[ "$1" == "-release" ]; then
    BUILD_TYPE="Release"
fi

# Detect CPU core count for parallel builds
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# Configure
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build with all cores
cmake --build build -j"$NPROC"
