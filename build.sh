#!/bin/bash
set -e

if [ "$1" == "clean" ] || [ "$1" == "-clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
    rm -rf .cache
fi

# Detect CPU core count for parallel builds
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# Configure
cmake -B build

# Build with all cores
cmake --build build -j"$NPROC"
