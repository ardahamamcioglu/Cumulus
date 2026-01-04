#!/bin/bash
set -e

# Configure the project
cmake -B build

# Build the project
cmake --build build
