# NPitaya Release Process

## Overview

This document describes the automated release process for NPitaya packages. The release workflow is triggered when a new tag is pushed to the repository and reuses the existing build artifacts from the `cmake.yml` workflow.

## Automated Release Options

### Option 1: Fully Automated (Recommended)
Use the `release-with-bump.yml` workflow which automatically:
1. Checks if version has already been updated by developer
2. Updates version references only if needed
3. Commits changes to master (only if version was updated)
4. Packages and publishes to Artifactory

### Option 2: PR-Based Version Bump
Use the `bump-version.yml` workflow which:
1. Creates a PR with version updates
2. Requires manual merge
3. Then run the release workflow

### Option 3: Manual Version Update
Manually update versions and then run the release workflow.

## Release Workflow

### Prerequisites
Before creating a release, ensure that:
1. The latest build has completed successfully (check GitHub Actions)
2. The `prebuilt-libs-all-archs` artifact is available from the latest build

### Trigger
The release workflow is triggered automatically when a tag is pushed:

```bash
git tag v1.0.7
git push origin v1.0.7
```

### Process

#### Option 1: Fully Automated Release
The `release-with-bump.yml` workflow performs the following steps:

1. **Version Check**: Checks if the version in `cpp-lib/version.txt` matches the tag
2. **Conditional Version Bump**: Updates version references only if they don't match
3. **Conditional Commit**: Commits changes to master only if version was updated
4. **Download Artifacts**: Downloads the latest `prebuilt-libs-all-archs` artifact
5. **Package Preparation**: Runs `package.sh` script to prepare the NPM package
6. **Publish**: Publishes the package to Artifactory

#### Option 2: PR-Based Release
The `bump-version.yml` workflow:
1. Creates a PR with version updates
2. Requires manual review and merge
3. After merge, run the release workflow manually

#### Option 3: Manual Release
1. **Update Version References**: Run `VERSION=1.0.7 ./update-version.sh`
2. **Commit Changes**: `git add . && git commit -m "Update version to 1.0.7"`
3. **Create Tag**: `git tag v1.0.7 && git push origin v1.0.7`
4. **Monitor Release**: The release workflow will package and publish automatically

## Build Workflow

The build workflow (`.github/workflows/cmake.yml`) runs on:
- Push to master branch
- Pull requests to master branch

This workflow builds native libraries for all supported platforms:
- Linux x86_64 (Ubuntu 22.04)
- Linux ARMv8 (Ubuntu 22.04)
- macOS x86_64
- macOS ARM64
- Windows x86_64

And creates a consolidated artifact: `prebuilt-libs-all-archs`

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

## Prerequisites

### Repository Secrets
The following secrets must be configured in the GitHub repository:

- `ARTIFACTORY_USER`: Artifactory username
- `ARTIFACTORY_PASS`: Artifactory password

### Tag Format
Tags should follow semantic versioning:
- `v1.0.7` (stable release)
- `v1.0.8-rc1` (release candidate)
- `v2.0.0-beta1` (beta release)

## Release Process

### Step 1: Ensure Build is Ready
Before creating a release, make sure the latest build has completed successfully:

1. Check the GitHub Actions tab
2. Verify that the `cmake.yml` workflow completed successfully
3. Confirm that the `prebuilt-libs-all-archs` artifact was created

### Step 2: Choose Release Method

#### Method A: Fully Automated (Recommended)
```bash
# Simply create and push a tag
git tag v1.0.7
git push origin v1.0.7
```
The workflow will automatically:
- Check if version needs updating
- Update version references only if needed
- Commit changes to master (only if version was updated)
- Package and publish to Artifactory

#### Method B: PR-Based
```bash
# Create tag to trigger PR creation
git tag v1.0.7
git push origin v1.0.7
# Review and merge the PR
# Then manually trigger release workflow
```

#### Method C: Manual
```bash
# Update versions manually
VERSION=1.0.7 ./update-version.sh

# Review changes
git diff

# Commit version updates
git add .
git commit -m "Update version to 1.0.7"

# Create and push tag
git tag v1.0.7
git push origin v1.0.7
```

### Step 3: Monitor Release
- Check the GitHub Actions tab to monitor the release process
- The package will be automatically published to Artifactory

## Package Script

The `package.sh` script performs the following operations:

1. **Creates Package Directory**: Sets up the package structure
2. **Copies Source Files**: Copies NPitaya C# source files
3. **Copies Libraries**: Copies built native libraries to appropriate runtime folders
4. **Updates Version**: Updates package.json with the release version
5. **Verifies Package**: Checks that all expected libraries are present

### Usage
```bash
VERSION=1.0.7 ./package.sh
```

## Smart Version Checking

The automated workflow includes intelligent version checking:

### How It Works
1. **Checks Current Version**: Reads `cpp-lib/version.txt` to get current version
2. **Compares with Tag**: Compares current version with the git tag
3. **Conditional Update**: Only updates if versions don't match
4. **Conditional Commit**: Only commits if version was actually updated

### Scenarios

#### Scenario 1: Developer Already Updated Version
```bash
# Developer manually updated version
VERSION=1.0.7 ./update-version.sh
git add . && git commit -m "Update version to 1.0.7"
git tag v1.0.7
git push origin v1.0.7
```
**Result**: ✅ Workflow skips version bump, proceeds directly to packaging

#### Scenario 2: Developer Forgot to Update Version
```bash
# Developer forgot to update version (still 1.0.6-rc4)
git tag v1.0.7
git push origin v1.0.7
```
**Result**: ✅ Workflow updates version, commits changes, then packages

#### Scenario 3: Mixed Version References
```bash
# Some files updated, others not
# version.txt: 1.0.7
# package.json: 1.0.6-rc4
git tag v1.0.7
git push origin v1.0.7
```
**Result**: ✅ Workflow detects mismatch, updates all files, commits changes

## Version Update Script

The `update-version.sh` script updates all version references across the repository:

1. **cpp-lib/version.txt**
2. **pitaya-sharp/NPitaya/package.json**
3. **pitaya-sharp/NPitaya-csproj/NPitaya.csproj**
4. **unity/NPitaya.nuspec**
5. **Any other files containing version references**

### Usage
```bash
VERSION=1.0.7 ./update-version.sh
```

## Troubleshooting

### Missing Build Artifacts
If the release fails because build artifacts are missing:

1. **Check Build Status**: Ensure the latest `cmake.yml` workflow completed successfully
2. **Wait for Build**: If a build is in progress, wait for it to complete
3. **Trigger Manual Build**: Push a commit to master to trigger a new build
4. **Retry Release**: Once the build completes, retry the release

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

### Version Bump Issues
If the automated version bump fails:
1. Check that the `update-version.sh` script is executable
2. Verify that all version files exist and are writable
3. Check the GitHub Actions logs for specific error messages

## Version Management

### Current Version
The current version is defined in `cpp-lib/version.txt`:
```
1.0.6
```

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

## Benefits of This Approach

1. **No Duplication**: Reuses existing build artifacts instead of rebuilding
2. **Faster Releases**: Release process is much faster since it doesn't rebuild
3. **Consistency**: Uses the same build artifacts that were tested in CI
4. **Simpler Maintenance**: Only one build workflow to maintain
5. **Automated Version Management**: No manual version updates required
6. **Consistent Commit Messages**: All version bumps follow the same format

## Support

For issues with the release process:
1. Check the GitHub Actions logs
2. Review this documentation
3. Contact the development team

## Changelog

Release notes should be updated in `CHANGELOG.md` before creating a new release tag.
