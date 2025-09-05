#!/bin/bash

# Complete Loom v1.0.0 Beta Package Script
# Builds, tests, and summarizes the DEB package in one go

set -e

VERSION="1.2.2"
PACKAGE_NAME="loom"
PACKAGE_FILE="${PACKAGE_NAME}_${VERSION}_amd64.deb"

echo "🚀 Loom Text Editor v${VERSION} Beta - Complete Package Build"
echo "============================================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    exit 1
fi

# Check for required tools
echo "🔧 Checking required tools..."
REQUIRED_TOOLS="cmake make dpkg-deb"
for tool in $REQUIRED_TOOLS; do
    if ! command -v $tool >/dev/null 2>&1; then
        echo "❌ Error: $tool is not installed. Please install it first:"
        echo "  sudo apt install cmake build-essential dpkg-dev"
        exit 1
    fi
done

# Check for build dependencies
echo "📦 Checking build dependencies..."
BUILD_DEPS="cmake qtbase5-dev liblua5.4-dev build-essential pkg-config"
MISSING_DEPS=""

for dep in $BUILD_DEPS; do
    if ! dpkg -l | grep -q "^ii  $dep"; then
        MISSING_DEPS="$MISSING_DEPS $dep"
    fi
done

if [ -n "$MISSING_DEPS" ]; then
    echo "❌ Missing build dependencies:$MISSING_DEPS"
    echo "Install them with:"
    echo "  sudo apt install$MISSING_DEPS"
    exit 1
fi

echo "✅ All dependencies satisfied"

# Clean previous builds
echo "🧹 Cleaning previous builds..."
rm -rf build/
rm -f ${PACKAGE_NAME}_*.deb

# Create build directory and configure
mkdir -p build
cd build

echo "⚙️ Configuring CMake for Release build..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      ..

# Build the project
echo "🔨 Building project..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build completed successfully"

# Test the binary
echo "🧪 Testing the built binary..."
if [ -f "./loom" ]; then
    echo "✅ Binary created successfully"
else
    echo "❌ Binary not found!"
    exit 1
fi

# Create the DEB package using CPack
echo "📦 Creating DEB package with CPack..."
cpack -G DEB

if [ $? -ne 0 ]; then
    echo "❌ Package build failed!"
    exit 1
fi

# Move the package to the root directory
mv ${PACKAGE_NAME}_*.deb ../
cd ..

echo "✅ Debian package built successfully!"

# =============================================================================
# TESTING PHASE
# =============================================================================

echo ""
echo "🧪 TESTING PHASE"
echo "================"

if [ ! -f "$PACKAGE_FILE" ]; then
    echo "❌ Package file not found: $PACKAGE_FILE"
    exit 1
fi

echo "📋 Running comprehensive package tests..."

# Test 1: Package integrity
echo "  🔍 Test 1: Package integrity..."
if dpkg-deb --fsys-tarfile "$PACKAGE_FILE" > /dev/null 2>&1; then
    echo "    ✅ Package integrity verified"
else
    echo "    ❌ Package integrity check failed"
    exit 1
fi

# Test 2: Package contents
echo "  📁 Test 2: Package contents..."
CONTENTS=$(dpkg-deb -c "$PACKAGE_FILE")

REQUIRED_FILES=(
    "./usr/bin/loom"
    "./usr/share/loom/config/"
    "./usr/share/loom/plugins/"
    "./usr/share/applications/loom.desktop"
    "./usr/share/pixmaps/loom.png"
)

for file in "${REQUIRED_FILES[@]}"; do
    if echo "$CONTENTS" | grep -q "$file"; then
        echo "    ✅ Found: $file"
    else
        echo "    ❌ Missing: $file"
        exit 1
    fi
done

# Test 3: Config and plugin files
echo "  ⚙️ Test 3: Config and plugin files..."
if echo "$CONTENTS" | grep -q "./usr/share/loom/config/config.lua"; then
    echo "    ✅ Found: config.lua"
else
    echo "    ❌ Missing: config.lua"
    exit 1
fi

if echo "$CONTENTS" | grep -q "./usr/share/loom/plugins/autoformat.lua"; then
    echo "    ✅ Found: autoformat.lua plugin"
else
    echo "    ❌ Missing: autoformat.lua plugin"
    exit 1
fi

if echo "$CONTENTS" | grep -q "./usr/share/loom/plugins/autosave.lua"; then
    echo "    ✅ Found: autosave.lua plugin"
else
    echo "    ❌ Missing: autosave.lua plugin"
    exit 1
fi

# Test 4: Package metadata
echo "  📋 Test 4: Package metadata..."
PACKAGE_INFO=$(dpkg-deb -I "$PACKAGE_FILE")

if echo "$PACKAGE_INFO" | grep -q "Package: loom"; then
    echo "    ✅ Package name correct"
else
    echo "    ❌ Package name incorrect"
    exit 1
fi

if echo "$PACKAGE_INFO" | grep -q "Version: 1.2.2"; then
    echo "    ✅ Package version correct"
else
    echo "    ❌ Package version incorrect"
    exit 1
fi

# Test 5: Binary verification
echo "  🔧 Test 5: Binary verification..."
TEMP_DIR=$(mktemp -d)
dpkg-deb -x "$PACKAGE_FILE" "$TEMP_DIR"
BINARY_FILE="$TEMP_DIR/usr/bin/loom"

if [ -f "$BINARY_FILE" ] && [ -x "$BINARY_FILE" ]; then
    if file "$BINARY_FILE" | grep -q "ELF.*executable.*x86-64"; then
        echo "    ✅ Binary is valid x86-64 ELF executable"
    else
        echo "    ❌ Binary is not a valid x86-64 ELF executable"
        exit 1
    fi
else
    echo "    ❌ Binary file not found or not executable"
    exit 1
fi

rm -rf "$TEMP_DIR"

echo "✅ All tests passed successfully!"

# =============================================================================
# SUMMARY PHASE
# =============================================================================

echo ""
echo "📊 PACKAGE SUMMARY"
echo "=================="

echo "📦 Package Information:"
echo "  File: $PACKAGE_FILE"
echo "  Size: $(du -h $PACKAGE_FILE | cut -f1)"
echo "  Created: $(date)"
echo ""

echo "🔍 Package Metadata:"
dpkg-deb -I $PACKAGE_FILE | grep -E "(Package|Version|Architecture|Maintainer|Description|Homepage|Depends|Recommends)" | sed 's/^/ /'
echo ""

echo "📁 Package Structure:"
echo "├── /usr/bin/"
echo "│   └── loom                    # Main executable"
echo "├── /usr/share/loom/"
echo "│   ├── config/"
echo "│   │   └── config.lua          # Default configuration"
echo "│   ├── plugins/"
echo "│   │   ├── autoformat.lua      # Code formatting plugin"
echo "│   │   └── autosave.lua        # Auto-save plugin"
echo "│   ├── themes/"
echo "│   │   └── gruvbox.qss         # Gruvbox theme"
echo "│   └── assets/"
echo "│       └── icon.png            # Application icon"
echo "├── /usr/share/applications/"
echo "│   └── loom.desktop            # Desktop entry"
echo "├── /usr/share/pixmaps/"
echo "│   └── loom.png                # System icon"
echo "└── /usr/share/doc/loom/"
echo "    └── README.md               # Documentation"
echo ""

echo "⚙️ Features Included:"
echo "  ✅ Fast, minimal text editor built with C++ and Qt"
echo "  ✅ Lua scripting for configuration and plugins"
echo "  ✅ Syntax highlighting for multiple programming languages"
echo "  ✅ Multi-tab interface with customizable keybindings"
echo "  ✅ Gruvbox theme for comfortable coding"
echo "  ✅ AutoSave plugin with configurable intervals"
echo "  ✅ AutoFormat plugin supporting:"
echo "      • C/C++ (clang-format)"
echo "      • JavaScript/TypeScript (prettier)"
echo "      • Python (black)"
echo "      • Lua (stylua)"
echo "      • JSON, HTML, CSS (prettier)"
echo "      • Rust (rustfmt)"
echo "  ✅ Desktop integration with .desktop file and icon"
echo "  ✅ Proper system installation paths"
echo "  ✅ User configuration override support"
echo ""

# Package statistics
TEMP_DIR=$(mktemp -d)
dpkg-deb -x $PACKAGE_FILE $TEMP_DIR

BINARY_SIZE=$(du -h $TEMP_DIR/usr/bin/loom | cut -f1)
CONFIG_FILES=$(find $TEMP_DIR/usr/share/loom/config -name "*.lua" | wc -l)
PLUGIN_FILES=$(find $TEMP_DIR/usr/share/loom/plugins -name "*.lua" | wc -l)
THEME_FILES=$(find $TEMP_DIR/usr/share/loom/themes -name "*.qss" | wc -l)

echo "📊 Package Statistics:"
echo "  Binary size: $BINARY_SIZE"
echo "  Configuration files: $CONFIG_FILES"
echo "  Plugin files: $PLUGIN_FILES"
echo "  Theme files: $THEME_FILES"
echo "  Total installed size: $(dpkg-deb -I $PACKAGE_FILE | grep 'Installed-Size' | cut -d: -f2 | xargs) KB"

rm -rf $TEMP_DIR

echo ""
echo "🚀 INSTALLATION INSTRUCTIONS"
echo "============================"
echo "# Install the package:"
echo "sudo dpkg -i $PACKAGE_FILE"
echo "sudo apt-get install -f"
echo ""
echo "# Run Loom:"
echo "loom [optional-file-path]"
echo ""

echo "🔧 Optional Formatters (for AutoFormat plugin):"
echo "sudo apt install clang-format nodejs npm python3-pip"
echo "npm install -g prettier"
echo "pip3 install black"
echo "cargo install stylua  # if Rust is installed"
echo ""
echo "# Or use the automated script:"
echo "./scripts/install_formatters.sh"
echo ""

echo "🌐 DISTRIBUTION"
echo "==============="
echo "Ready for upload to GitHub releases:"
echo "https://github.com/dexter-xd/loom/releases"
echo ""

echo "🎉 SUCCESS!"
echo "==========="
echo "✅ Loom v${VERSION} Beta package created successfully!"
echo "✅ All tests passed"
echo "✅ Package is ready for distribution"
echo ""
echo "Package: $PACKAGE_FILE"
echo "Status: 🚀 Production Ready"
