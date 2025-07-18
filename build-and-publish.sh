#!/bin/bash

# Script to build and publish libpitaya for Ubuntu 22.04+ compatibility
set -e

echo "=== Building libpitaya for Ubuntu 22.04+ compatibility ==="

# Step 1: Build the Linux library using Docker
echo "Building Linux library in Docker container (Ubuntu 22.04)..."
cd cpp-lib
make build-linux-docker
cd ..

# Step 2: Check if the library was built successfully
if [ ! -f "cpp-lib/_builds/linux-x86_64-release/libpitaya_cpp.so" ]; then
    echo "Error: Linux library not found!"
    exit 1
fi

# Step 3: Copy the library to the correct location
echo "Copying library to pitaya-sharp plugins..."
cp cpp-lib/_builds/linux-x86_64-release/libpitaya_cpp.so pitaya-sharp/NPitaya/Runtime/Plugins/runtimes/linux-x86_64/

# Step 4: Check GLIBC version requirements
echo "Checking GLIBC version requirements..."
if [ -f "cpp-lib/check_glibc.sh" ]; then
    cd cpp-lib
    chmod +x check_glibc.sh
    ./check_glibc.sh
    cd ..
fi

# Step 5: Build the C# library
echo "Building C# library..."
cd pitaya-sharp
make build
cd ..

echo "=== Build completed successfully! ==="
echo "Library location: pitaya-sharp/NPitaya/Runtime/Plugins/runtimes/linux-x86_64/libpitaya_cpp.so"
echo ""
echo "Compatibility:"
echo "- Ubuntu 22.04 (jammy): GLIBC 2.35 ✅"
echo "- Ubuntu 23.04 (lunar): GLIBC 2.37 ✅"
echo "- Ubuntu 23.10 (mantic): GLIBC 2.38 ✅"
echo "- Ubuntu 24.04 (noble): GLIBC 2.39 ✅"
echo ""
echo "Benefits:"
echo "- Ubuntu 22.04 LTS support until April 2027"
echo "- Stable, well-tested dependency versions"
echo "- Maximum compatibility with current LTS versions"
echo ""
echo "To publish:"
echo "1. Commit your changes"
echo "2. Push to trigger GitHub CI build"
echo "3. Or manually copy the library to your deployment"
