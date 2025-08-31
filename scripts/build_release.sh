#!/bin/bash

# Build script for release version (no debug logs)
echo "Building Loom Text Editor - Release Version"
echo "=========================================="

mkdir -p build
cd build

echo "Configuring for Release build..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "Building..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Release build completed successfully!"
    echo "✓ Debug logs are disabled for optimal performance"
    echo "✓ Config and plugins folders copied to build directory"
    echo ""
    echo "Run with: ./run_loom.sh [file]  (recommended - suppresses Qt warnings)"
    echo "Or run with: ./build/loom [file]"
else
    echo "✗ Build failed!"
    exit 1
fi