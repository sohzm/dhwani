#!/bin/bash
set -e

# Output directories
mkdir -p build/linux
mkdir -p build/windows

echo "🔨 Building for Linux..."
gcc dhwani.c -o build/linux/dhwani -lpthread -lm
echo "✅ Linux build complete: build/linux/my_app"

echo "🔨 Building for Windows..."
x86_64-w64-mingw32-gcc dhwani.c -o build/windows/dhwani.exe -lwinmm
echo "✅ Windows build complete: build/windows/my_app.exe"
