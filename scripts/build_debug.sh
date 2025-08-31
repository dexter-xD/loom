#!/bin/bash

# Build script for debug version (with debug logs)
echo "Building Loom Text Editor - Debug Version"
echo "========================================"

mkdir -p build
cd build

echo "Configuring for Debug build..."
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo "Building..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Debug build completed successfully!"
    echo "✓ Debug logs are enabled for development"
    echo "✓ Config and plugins folders copied to build directory"
    echo ""
    echo "Run with: ./run_loom.sh [file]  (recommended - suppresses Qt warnings)"
    echo "Or run with: ./build/loom [file]"
else
    echo "✗ Build failed!"
    exit 1
fi