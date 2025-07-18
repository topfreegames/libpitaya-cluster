# Ubuntu 22.04 Compatibility Changes

## Overview

This document outlines the changes made to ensure libpitaya builds are compatible with Ubuntu 22.04 (jammy) runtime environments while maintaining compatibility with newer Ubuntu versions.

## Problem

The previous build configuration was creating libraries that required GLIBC 2.38, which is only available in Ubuntu 23.10+. This caused runtime errors on Ubuntu 22.04 environments.

## Solution

### 1. GitHub CI Configuration (.github/workflows/cmake.yml)

**Changes:**
- Pinned Ubuntu runners from `ubuntu-latest` to `ubuntu-22.04`
- Ensures consistent build environment using Ubuntu 22.04

**Benefits:**
- Builds are now created in Ubuntu 22.04 environment
- Libraries will be compatible with Ubuntu 22.04+ runtime

### 2. Dependency Downgrades (cpp-lib/conanfile.py)

**Changes:**
- **OpenSSL**: Downgraded from `3.4.1` to `3.1.4`
- **Boost**: Downgraded from `1.86.0` to `1.82.0`

**Why:**
- OpenSSL 3.4.1 requires GLIBC 2.38
- OpenSSL 3.1.4 is compatible with GLIBC 2.35 (Ubuntu 22.04)
- Boost 1.82.0 is more stable and compatible with older GLIBC versions

### 3. Docker Build Environment (cpp-lib/Dockerfile)

**Changes:**
- Updated from `conanio/clang14-ubuntu18.04` to `ubuntu:22.04`
- Added manual installation of build tools
- Ensures local Docker builds match CI environment

### 4. Build Scripts

**Changes:**
- Updated `build-and-publish.sh` to reflect Ubuntu 22.04 compatibility
- Created `cpp-lib/check_glibc.sh` for GLIBC version verification
- Updated Makefile comments for clarity

## Compatibility Matrix

| Ubuntu Version | GLIBC Version | Compatibility |
|----------------|---------------|---------------|
| Ubuntu 22.04 (jammy) | 2.35 | ✅ Target |
| Ubuntu 23.04 (lunar) | 2.37 | ✅ Compatible |
| Ubuntu 23.10 (mantic) | 2.38 | ✅ Compatible |
| Ubuntu 24.04 (noble) | 2.39 | ✅ Compatible |

## Verification

To verify GLIBC compatibility:

```bash
# Build the library
./build-and-publish.sh

# Check GLIBC requirements
cd cpp-lib
./check_glibc.sh
```

Expected output should show GLIBC requirements of 2.35 or lower.

## Benefits

1. **Runtime Compatibility**: Libraries now work on Ubuntu 22.04+
2. **Forward Compatibility**: Still compatible with newer Ubuntu versions
3. **Consistent Builds**: CI and local builds use same environment
4. **Stable Dependencies**: Using more stable versions of OpenSSL and Boost

## Migration Guide

### For Runtime Environments

1. **Ubuntu 22.04+**: No changes needed, libraries will work
2. **Older Ubuntu versions**: Update to Ubuntu 22.04 or newer

### For Development

1. **Local builds**: Use `./build-and-publish.sh`
2. **CI builds**: Automatic via GitHub Actions
3. **Verification**: Use `cpp-lib/check_glibc.sh`

## Rollback Plan

If issues arise with the downgraded dependencies:

1. Revert conanfile.py to previous versions
2. Update runtime environments to Ubuntu 23.10+
3. Update CI configuration to use Ubuntu 23.10 runners

## Testing

Test the compatibility by:

1. Building on Ubuntu 22.04
2. Running on Ubuntu 22.04, 23.04, 23.10, 24.04
3. Verifying no GLIBC version errors occur
