# NPitaya Release Process

## Overview

This document describes the automated release process for NPitaya packages. The release workflow is triggered when a new tag is pushed to the repository and automatically builds, packages, and publishes the package to Artifactory.

## Release Methods

### Method 1: Make Command (Recommended)
Use the `make release` command which automatically:
1. Checks GitHub CLI installation and authentication
2. Updates version references across all files
3. Commits and pushes changes to the current branch
4. Creates a git tag
5. Creates a GitHub release with auto-generated notes
6. Triggers the automated build and publish workflow

### Method 2: Manual Process
Manually update versions, create tags, and trigger releases through the GitHub UI.

## Prerequisites

### GitHub CLI Installation
The recommended release method requires GitHub CLI (`gh`). If not installed:

**macOS:**
```bash
brew install gh
```

**Ubuntu/Debian:**
```bash
sudo apt install gh
```

**Windows:**
```bash
winget install GitHub.cli
```

**Or download from:** https://cli.github.com/

### GitHub CLI Authentication
After installation, authenticate with GitHub:

```bash
gh auth login
```

Follow the prompts to complete authentication.

### Repository Secrets
The following secrets must be configured in the GitHub repository:

- `ARTIFACTORY_USER`: Artifactory username
- `ARTIFACTORY_PASS`: Artifactory password

## Release Workflow

### Automated Workflow Process
The `build-and-release.yml` workflow performs the following steps:

1. **Cache Check**: Checks if consolidated libraries are cached (skips build if available)
2. **Build**: Builds native libraries for all supported platforms (if cache miss)
3. **Consolidate**: Processes and consolidates libraries from all platforms
4. **Package**: Creates the NPM package with the correct version
5. **Publish**: Publishes the package to Artifactory

### Trigger
The release workflow is triggered automatically when a tag is pushed to the repository.

## Release Process

### Method 1: Make Command (Recommended)

#### Step 1: Ensure Build is Ready
Before creating a release, make sure your changes are ready:
1. All code changes are committed and pushed
2. Tests pass locally
3. You're on the correct branch

#### Step 2: Run the Release Command
```bash
make release VERSION=v1.0.7
```

This command will:
1. ✅ Check if GitHub CLI is installed
2. ✅ Verify GitHub CLI authentication
3. ✅ Update version references in all files
4. ✅ Commit and push changes to current branch
5. ✅ Create and push git tag `v1.0.7`
6. ✅ Create GitHub release with auto-generated notes
7. ✅ Trigger automated build and publish workflow

#### Step 3: Monitor the Process
- Check the GitHub Actions tab to monitor the build and publish process
- The package will be automatically published to Artifactory

### Method 2: Manual Process

#### Step 1: Update Version References
```bash
VERSION=v1.0.7 ./update-version.sh
```

#### Step 2: Commit and Push Changes
```bash
git add .
git commit -m "chore: update version to v1.0.7"
git push origin HEAD
```

#### Step 3: Create Tag and Release
```bash
# Create and push tag
git tag v1.0.7
git push origin v1.0.7

# Create GitHub release (if gh is available)
gh release create v1.0.7 --title "Release v1.0.7" --generate-notes
```

#### Step 4: Monitor Release
- Check the GitHub Actions tab to monitor the release process
- The package will be automatically published to Artifactory

## Build Workflow

The build workflow (`.github/workflows/build-and-release.yml`) runs on:
- Tag push (automatically triggered by release process)

This workflow builds native libraries for all supported platforms:
- Linux x86_64 (Ubuntu 22.04)
- Linux ARMv8 (Ubuntu 22.04)
- macOS x86_64
- macOS ARM64
- Windows x86_64

The workflow includes intelligent caching:
- **Conan Cache**: Caches build dependencies per platform
- **Consolidated Cache**: Caches processed libraries (skips build if no changes)
- **Smart Invalidation**: Cache invalidates when cpp-lib/vendor files change

## Package Structure

The final package structure follows the Unity package format:

```
package/
├── package.json
├── Runtime/
│   ├── Plugins/
│   │   └── runtimes/
│   │       ├── linux-x86_64/
│   │       │   └── libpitaya_cpp.so
│   │       ├── linux-armv8/
│   │       │   └── libpitaya_cpp.so
│   │       ├── macos-x86_64/
│   │       │   └── libpitaya_cpp.dylib
│   │       ├── macos-arm64/
│   │       │   └── libpitaya_cpp.dylib
│   │       └── windows-x86_64/
│   │           └── libpitaya_cpp.dll
│   ├── *.cs (C# source files)
│   └── ... (other Unity package files)
```

### NPM Version Format
Versions should follow semantic versioning:
- `1.0.7` (stable release)
- `1.0.8-rc1` (release candidate)
- `2.0.0-beta1` (beta release)

The `v` prefix is removed for NPM release

## Make Commands

### Available Commands

#### `make check-gh`
Checks if GitHub CLI is installed and provides installation instructions if not.

#### `make signin-gh`
Checks GitHub CLI authentication and prompts to sign in if needed.

#### `make release VERSION=x.y.z`
Complete release process:
- Checks GitHub CLI installation and authentication
- Updates version references in all files
- Commits and pushes changes
- Creates git tag
- Creates GitHub release
- Triggers automated build and publish

### Examples
```bash
# Check GitHub CLI setup
make check-gh

# Sign in to GitHub CLI
make signin-gh

# Create a release candidate
make release VERSION=v1.0.8-rc1
```

## Package Script

The `package.sh` script performs the following operations:

1. **Creates Package Directory**: Sets up the package structure
2. **Copies Source Files**: Copies NPitaya C# source files
3. **Copies Libraries**: Copies built native libraries to appropriate runtime folders
4. **Updates Version**: Updates package.json with the release version
5. **Verifies Package**: Checks that all expected libraries are present

### Usage
```bash
VERSION=v1.0.7 ./package.sh
```

## Workflow Optimization

The release workflow includes intelligent caching to optimize build times:

### Cache Strategy
- **Conan Cache**: Caches build dependencies per platform/architecture
- **Consolidated Cache**: Caches processed libraries across all platforms
- **Smart Invalidation**: Cache invalidates when source files or dependencies change

### Cache Hit Scenario
When no changes are made to cpp-lib or vendor files:
1. ✅ Cache check finds existing consolidated libraries
2. ✅ Skips entire build process
3. ✅ Proceeds directly to packaging and publishing
4. ✅ Significantly faster release process

### Cache Miss Scenario
When changes are made to cpp-lib or vendor files:
1. ✅ Cache check finds no valid cache
2. ✅ Runs full build for all platforms
3. ✅ Processes and consolidates libraries
4. ✅ Caches results for future releases
5. ✅ Proceeds to packaging and publishing

## Version Update Script

The `update-version.sh` script updates all version references across the repository:

1. **cpp-lib/version.txt**
2. **pitaya-sharp/NPitaya/package.json**
3. **pitaya-sharp/NPitaya-csproj/NPitaya.csproj**
4. **unity/NPitaya.nuspec**
5. **Any other files containing version references**

### Usage
```bash
VERSION=v1.0.7 ./update-version.sh
```

## Troubleshooting

### GitHub CLI Issues
If the make release command fails:

1. **Installation Issues**: Run `make check-gh` for installation instructions
2. **Authentication Issues**: Run `make signin-gh` to check and fix authentication
3. **Permission Issues**: Ensure your GitHub account has push access to the repository

### Build Failures
If the build workflow fails:
1. Check the GitHub Actions logs for specific error messages
2. Verify that all dependencies are available
3. Check that the build environment is properly configured

### Publishing Issues
If publishing to Artifactory fails:
1. Verify that `ARTIFACTORY_USER` and `ARTIFACTORY_PASS` secrets are set
2. Check that the user has publish permissions
3. Verify that the package version doesn't already exist

### Manual Release Issues
If manual release process fails:
1. Check that the `update-version.sh` script is executable
2. Verify that all version files exist and are writable
3. Ensure you have push access to create tags and releases

## Version Management

### Version Updates
When creating a new release:
1. The automated workflow will update all version references
2. Changes will be committed to master
3. The package will be published with the new version

## Artifactory Configuration

The package is published to:
- **Registry**: `https://artifactory.tfgco.com/artifactory/api/npm/npm-local`
- **Scope**: `@wls`
- **Package Name**: `com.wildlifestudios.npitaya`

## Support

For issues with the release process:
1. Check the GitHub Actions logs
2. Review this documentation
3. Contact the development team

## Changelog

Release notes should be updated in `CHANGELOG.md` before creating a new release tag.
