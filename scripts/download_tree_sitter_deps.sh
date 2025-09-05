#!/bin/bash

# Tree-sitter Dependencies Downloader
# Downloads Tree-sitter language grammars to external/ folder for development

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$PROJECT_ROOT/external"

echo "Tree-sitter Dependencies Downloader"
echo "==================================="
echo "Project root: $PROJECT_ROOT"
echo "External dir: $EXTERNAL_DIR"
echo ""

# Create external directory if it doesn't exist
mkdir -p "$EXTERNAL_DIR"

# Change to external directory
cd "$EXTERNAL_DIR"

echo "Downloading Tree-sitter dependencies..."
echo ""

# Tree-sitter core library
echo "📦 Downloading tree-sitter (core library)..."
if [ ! -d "tree-sitter" ]; then
    git clone https://github.com/tree-sitter/tree-sitter.git
    echo "✅ tree-sitter downloaded"
else
    echo "⚠️  tree-sitter already exists, skipping"
fi
echo ""

# Language grammars
declare -A repos=(
    ["tree-sitter-c"]="https://github.com/tree-sitter/tree-sitter-c.git"
    ["tree-sitter-cpp"]="https://github.com/tree-sitter/tree-sitter-cpp.git"
    ["tree-sitter-javascript"]="https://github.com/tree-sitter/tree-sitter-javascript.git"
    ["tree-sitter-rust"]="https://github.com/tree-sitter/tree-sitter-rust.git"
    ["tree-sitter-java"]="https://github.com/tree-sitter/tree-sitter-java.git"
    ["tree-sitter-go"]="https://github.com/tree-sitter/tree-sitter-go.git"
    ["tree-sitter-python"]="https://github.com/tree-sitter/tree-sitter-python.git"
    ["tree-sitter-lua"]="https://github.com/tree-sitter-grammars/tree-sitter-lua.git"
)

for repo in "${!repos[@]}"; do
    echo "📦 Downloading $repo..."
    if [ ! -d "$repo" ]; then
        git clone "${repos[$repo]}" "$repo"
        echo "✅ $repo downloaded"
    else
        echo "⚠️  $repo already exists, skipping"
    fi
    echo ""
done

echo "🎉 All Tree-sitter dependencies downloaded successfully!"
echo ""
echo "Next steps:"
echo "1. Run CMake to configure the build: mkdir build && cd build && cmake .."
echo "2. Build the project: make -j$(nproc)"
echo "3. Test with sample files: ./loom ../test_files/sample.cpp"
echo ""
echo "Note: The external/ folder is gitignored to avoid committing large binaries."
