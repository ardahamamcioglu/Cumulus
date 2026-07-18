#!/bin/bash
set -e

if [ "$1" == "clean" ] || [ "$1" == "-clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
    rm -rf .cache
fi

# Configure the project
cmake -B build

# Build the project
cmake --build build
