#!/bin/bash

# Script to package NPitaya for NPM release
set -e

# Check if VERSION is provided
if [ -z "$VERSION" ]; then
    echo "Error: VERSION environment variable is required"
    echo "Usage: VERSION=v1.0.0 ./package.sh"
    echo "       VERSION=1.0.0 ./package.sh"
    exit 1
fi

# Strip 'v' prefix if present for NPM package
VERSION_CLEAN=$(echo "$VERSION" | sed 's/^v//')
echo "=== Packaging NPitaya version $VERSION_CLEAN (from $VERSION) ==="

# Create package directory
PACKAGE_DIR="package"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

# Copy NPitaya source files
echo "Copying NPitaya source files..."
cp -r pitaya-sharp/NPitaya/* "$PACKAGE_DIR/"

# Create runtime directories if they don't exist
mkdir -p "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-x86_64"
mkdir -p "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-armv8"
mkdir -p "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-x86_64"
mkdir -p "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-arm64"
mkdir -p "$PACKAGE_DIR/Runtime/Plugins/runtimes/windows-x86_64"

# Copy built libraries from artifacts
echo "Copying built libraries..."

# Linux libraries
if [ -f "downloaded-artifacts/linux-x86_64/libpitaya_cpp.so" ]; then
    echo "  - Linux x86_64 library"
    cp downloaded-artifacts/linux-x86_64/libpitaya_cpp.so "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-x86_64/"
fi

if [ -f "downloaded-artifacts/linux-armv8/libpitaya_cpp.so" ]; then
    echo "  - Linux ARMv8 library"
    cp downloaded-artifacts/linux-armv8/libpitaya_cpp.so "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-armv8/"
fi

# macOS libraries
if [ -f "downloaded-artifacts/macos-x86_64/libpitaya_cpp.dylib" ]; then
    echo "  - macOS x86_64 library"
    cp downloaded-artifacts/macos-x86_64/libpitaya_cpp.dylib "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-x86_64/"
fi

if [ -f "downloaded-artifacts/macos-arm64/libpitaya_cpp.dylib" ]; then
    echo "  - macOS ARM64 library"
    cp downloaded-artifacts/macos-arm64/libpitaya_cpp.dylib "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-arm64/"
fi

# Windows library
if [ -f "downloaded-artifacts/windows-x86_64/libpitaya_cpp.dll" ]; then
    echo "  - Windows x86_64 library"
    cp downloaded-artifacts/windows-x86_64/libpitaya_cpp.dll "$PACKAGE_DIR/Runtime/Plugins/runtimes/windows-x86_64/"
fi

# Update package.json version
echo "Updating package.json version to $VERSION_CLEAN..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS requires an empty string for -i
    sed -i '' "s/\"version\": \".*\"/\"version\": \"$VERSION_CLEAN\"/" "$PACKAGE_DIR/package.json"
else
    # Linux
    sed -i "s/\"version\": \".*\"/\"version\": \"$VERSION_CLEAN\"/" "$PACKAGE_DIR/package.json"
fi

# Verify the package structure
echo ""
echo "=== Package Structure ==="
echo "Package directory: $PACKAGE_DIR"
echo "Package.json version: $(grep '"version"' "$PACKAGE_DIR/package.json")"
echo ""
echo "Runtime libraries:"
find "$PACKAGE_DIR/Runtime/Plugins/runtimes" -name "*.so" -o -name "*.dylib" -o -name "*.dll" | sort

# Check if all expected libraries are present
echo ""
echo "=== Library Verification ==="
MISSING_LIBS=0

check_library() {
    local lib_path="$1"
    local lib_name="$2"
    if [ -f "$lib_path" ]; then
        echo "✅ $lib_name"
    else
        echo "❌ $lib_name (missing)"
        MISSING_LIBS=$((MISSING_LIBS + 1))
    fi
}

check_library "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-x86_64/libpitaya_cpp.so" "Linux x86_64"
check_library "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-armv8/libpitaya_cpp.so" "Linux ARMv8"
check_library "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-x86_64/libpitaya_cpp.dylib" "macOS x86_64"
check_library "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-arm64/libpitaya_cpp.dylib" "macOS ARM64"
check_library "$PACKAGE_DIR/Runtime/Plugins/runtimes/windows-x86_64/libpitaya_cpp.dll" "Windows x86_64"

if [ $MISSING_LIBS -gt 0 ]; then
    echo ""
    echo "⚠️  Warning: $MISSING_LIBS library(ies) are missing!"
    echo "The package will still be created but may not work on all platforms."
fi

echo ""
echo "=== Package Ready ==="
echo "Package created in: $PACKAGE_DIR"
echo "Version: $VERSION_CLEAN"
echo ""
echo "To publish:"
echo "cd $PACKAGE_DIR && npm publish"
