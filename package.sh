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

# In CI, we always expect `downloaded-artifacts/` to be present (from the workflow).
# If it's missing, or if a library file isn't found, the old behavior was to silently
# ship the already-committed binaries from pitaya-sharp/NPitaya/Runtime/Plugins/.
# That can publish stale native libraries even when C++ code changed.
ARTIFACTS_DIR="downloaded-artifacts"
# Some historical workflow configurations uploaded the artifact with a leading
# `downloaded-artifacts/` path prefix, which results in a nested directory after
# download: downloaded-artifacts/downloaded-artifacts/...
ARTIFACTS_DIR_NESTED="downloaded-artifacts/downloaded-artifacts"

pick_artifacts_dir() {
    if [ -d "$ARTIFACTS_DIR" ]; then
        echo "$ARTIFACTS_DIR"
        return 0
    fi
    if [ -d "$ARTIFACTS_DIR_NESTED" ]; then
        echo "$ARTIFACTS_DIR_NESTED"
        return 0
    fi
    return 1
}

if ! EFFECTIVE_ARTIFACTS_DIR=$(pick_artifacts_dir); then
    echo "Error: no downloaded artifacts directory found."
    echo "Expected either '$ARTIFACTS_DIR/' or '$ARTIFACTS_DIR_NESTED/'"
    exit 1
fi

echo "Using artifacts directory: $EFFECTIVE_ARTIFACTS_DIR"

copy_or_fail() {
    local src="$1"
    local dst_dir="$2"
    local label="$3"
    if [ -f "$src" ]; then
        echo "  - $label ($(shasum -a 256 "$src" | awk '{print $1}'))"
        cp "$src" "$dst_dir/"
    else
        echo "Error: missing expected artifact: $src"
        exit 1
    fi
}

# Linux libraries
copy_or_fail "$EFFECTIVE_ARTIFACTS_DIR/linux-x86_64/libpitaya_cpp.so" \
             "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-x86_64" \
             "Linux x86_64 library"

copy_or_fail "$EFFECTIVE_ARTIFACTS_DIR/linux-armv8/libpitaya_cpp.so" \
             "$PACKAGE_DIR/Runtime/Plugins/runtimes/linux-armv8" \
             "Linux ARMv8 library"

# macOS libraries
copy_or_fail "$EFFECTIVE_ARTIFACTS_DIR/macos-x86_64/libpitaya_cpp.dylib" \
             "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-x86_64" \
             "macOS x86_64 library"

copy_or_fail "$EFFECTIVE_ARTIFACTS_DIR/macos-arm64/libpitaya_cpp.dylib" \
             "$PACKAGE_DIR/Runtime/Plugins/runtimes/macos-arm64" \
             "macOS ARM64 library"

# Windows library
copy_or_fail "$EFFECTIVE_ARTIFACTS_DIR/windows-x86_64/libpitaya_cpp.dll" \
             "$PACKAGE_DIR/Runtime/Plugins/runtimes/windows-x86_64" \
             "Windows x86_64 library"

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
