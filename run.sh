#!/bin/bash
set -e

if [ "$1" == "-v" ]; then
	./build/Cumulus.app/Contents/MacOS/Cumulus
else
	open ./build/Cumulus.app
fi
