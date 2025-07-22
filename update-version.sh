#!/bin/bash

# Script to update all version references across the repository
set -e

# Check if VERSION is provided
if [ -z "$VERSION" ]; then
    echo "Error: VERSION environment variable is required"
    echo "Usage: VERSION=v1.0.7 ./update-version.sh"
    echo "       VERSION=1.0.7 ./update-version.sh"
    exit 1
fi

# Strip 'v' prefix if present for internal use
VERSION_CLEAN=$(echo "$VERSION" | sed 's/^v//')
echo "=== Updating version references to $VERSION_CLEAN (from $VERSION) ==="

# Function to update version in package.json
update_package_json() {
    local file_path="$1"
    if [ -f "$file_path" ]; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS requires an empty string for -i
            sed -i '' 's/"version": "[^"]*"/"version": "'"$VERSION_CLEAN"'"/' "$file_path"
        else
            # Linux
            sed -i 's/"version": "[^"]*"/"version": "'"$VERSION_CLEAN"'"/' "$file_path"
        fi
        echo "✅ Updated: $file_path"
    else
        echo "⚠️  File not found: $file_path"
    fi
}

# Function to update version in .csproj file
update_csproj() {
    local file_path="$1"
    if [ -f "$file_path" ]; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS requires an empty string for -i
            sed -i '' 's/<PackageVersion>[^<]*<\/PackageVersion>/<PackageVersion>'"$VERSION_CLEAN"'<\/PackageVersion>/' "$file_path"
        else
            # Linux
            sed -i 's/<PackageVersion>[^<]*<\/PackageVersion>/<PackageVersion>'"$VERSION_CLEAN"'<\/PackageVersion>/' "$file_path"
        fi
        echo "✅ Updated: $file_path"
    else
        echo "⚠️  File not found: $file_path"
    fi
}

# Function to update version in .nuspec file
update_nuspec() {
    local file_path="$1"
    if [ -f "$file_path" ]; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS requires an empty string for -i
            sed -i '' 's/<version>[^<]*<\/version>/<version>'"$VERSION_CLEAN"'<\/version>/' "$file_path"
        else
            # Linux
            sed -i 's/<version>[^<]*<\/version>/<version>'"$VERSION_CLEAN"'<\/version>/' "$file_path"
        fi
        echo "✅ Updated: $file_path"
    else
        echo "⚠️  File not found: $file_path"
    fi
}

# Update version.txt
if [ -f "cpp-lib/version.txt" ]; then
    echo "$VERSION_CLEAN" > cpp-lib/version.txt
    echo "✅ Updated: cpp-lib/version.txt"
else
    echo "⚠️  File not found: cpp-lib/version.txt"
fi

echo ""
echo "Updating version files..."

# Update all version references
update_package_json "pitaya-sharp/NPitaya/package.json"
update_csproj "pitaya-sharp/NPitaya-csproj/NPitaya.csproj"
update_nuspec "unity/NPitaya.nuspec"

echo ""
echo "=== Version Update Complete ==="
echo "Updated to $VERSION_CLEAN in:"
echo "✅ cpp-lib/version.txt"
echo "✅ pitaya-sharp/NPitaya/package.json"
echo "✅ pitaya-sharp/NPitaya-csproj/NPitaya.csproj"
echo "✅ unity/NPitaya.nuspec"
echo ""
