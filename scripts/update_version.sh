#!/bin/bash

# Version Update Script for Loom Text Editor
# Updates version numbers across all project files

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <new_version>"
    echo "Example: $0 1.2.7"
    exit 1
fi

NEW_VERSION="$1"
OLD_VERSION=$(grep -oP 'project\(loom VERSION \K[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt)

echo "🔄 Updating Loom version from $OLD_VERSION to $NEW_VERSION"
echo "=============================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    exit 1
fi

# Validate version format
if ! [[ $NEW_VERSION =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "❌ Error: Version must be in format X.Y.Z (e.g., 1.2.6)"
    exit 1
fi

echo "📝 Updating version in project files..."

# 1. Update CMakeLists.txt
echo "  ✅ CMakeLists.txt"
sed -i "s/project(loom VERSION $OLD_VERSION LANGUAGES CXX C)/project(loom VERSION $NEW_VERSION LANGUAGES CXX C)/" CMakeLists.txt

# 2. Update main.cpp
echo "  ✅ src/main.cpp"
sed -i "s/app.setApplicationVersion(\".*\")/app.setApplicationVersion(\"$NEW_VERSION\")/" src/main.cpp

# 3. Update About dialog
echo "  ✅ src/editor_window/editor_window_menus.cpp"
sed -i "s/<p>Version .*<\/p>/<p>Version $NEW_VERSION<\/p>/" src/editor_window/editor_window_menus.cpp

# 4. Update package.sh script
echo "  ✅ scripts/package.sh"
sed -i "s/VERSION=\"$OLD_VERSION\"/VERSION=\"$NEW_VERSION\"/" scripts/package.sh
sed -i "s/grep -q \"Version: $OLD_VERSION\"/grep -q \"Version: $NEW_VERSION\"/" scripts/package.sh

# 5. Update README.md
echo "  ✅ README.md"
sed -i "s/beta_$OLD_VERSION/beta_$NEW_VERSION/g" README.md
sed -i "s/loom_$OLD_VERSION/loom_$NEW_VERSION/g" README.md

echo ""
echo "📋 Updated files:"
echo "  • CMakeLists.txt - Project version"
echo "  • src/main.cpp - Application version"
echo "  • src/editor_window/editor_window_menus.cpp - About dialog"
echo "  • scripts/package.sh - Package script version"
echo "  • README.md - Download links and package names"
echo ""
echo "⚠️  Manual update required:"
echo "  • debian/changelog - Add new changelog entry"
echo ""
echo "✅ Version update completed: $OLD_VERSION → $NEW_VERSION"
echo ""
echo "📝 Next steps:"
echo "1. Update debian/changelog with release notes"
echo "2. Commit changes: git add . && git commit -m \"Bump version to $NEW_VERSION\""
echo "3. Create tag: git tag beta_$NEW_VERSION"
echo "4. Build package: ./scripts/package.sh"
echo "5. Push changes: git push && git push --tags"