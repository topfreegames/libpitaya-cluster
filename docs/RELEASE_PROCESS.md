# NPitaya Release Process

## Overview

NPitaya releases are triggered by pushing a tag to the repository. The `build-and-release.yml` GitHub Actions workflow then builds native libraries for 5 platforms, packages the Unity/npm artifact, and publishes it to Artifactory. OpenUPM picks up the new tag automatically a few minutes later.

**Cut releases via the GitHub Releases UI — not from a developer machine.** Local commands like `git push origin <tag>` and `make release` exist as a fallback but should not be used for routine releases. They bypass code review of the version bump, skip generated release notes, and produce inconsistent state when something goes wrong. See [Recommended Release Flow](#recommended-release-flow).

This document is aimed at maintainers cutting releases. Contributors making changes to the project should open PRs as usual; the act of releasing is a separate, gated step.

## Versioning Conventions

NPitaya follows [semantic versioning](https://semver.org). Tags use the format `vX.Y.Z`, with an optional pre-release suffix. The `v` prefix is required for the git tag and is stripped automatically when packaging for npm.

### Suffix convention

- `vX.Y.Z` — stable release.
- `vX.Y.Z-rc.1`, `vX.Y.Z-rc.2`, … — release candidates used for staging before a stable release.
- `vX.Y.Z-alpha.1`, `vX.Y.Z-alpha.2`, … — early prereleases used on feature branches for integration testing.

### Pre-release ordering (important)

Under semver pre-release rules: **`alpha < beta < rc < (no suffix)`**. This means `1.1.0-alpha.1` sorts *before* the already-shipped `1.1.0`, and a consumer resolving `^1.1.0` will skip it.

**Rule:** once `vX.Y.Z` (no suffix) has shipped, do not publish `vX.Y.Z-alpha.N` or `vX.Y.Z-rc.N` against the same `X.Y.Z`. Bump the next minor or patch instead. Example: after `v1.1.0` ships, the next prerelease is `v1.2.0-alpha.1` (next minor) or `v1.1.1-alpha.1` (next patch line).

### Branch policy

The workflow triggers on any tag push, regardless of branch (`tags: "*"`). Convention:

- **Stable releases (`vX.Y.Z`)**: target `master` after the change is merged.
- **Release candidates and alphas**: target a feature branch when you need an integration build for downstream consumers. The published artifact reflects the tagged commit, not master.

## Prerequisites

The release flow runs in GitHub Actions. The following must be configured at the repository level:

### Repository secrets

In GitHub → Settings → Secrets and variables → Actions:

- `ARTIFACTORY_USER` — Artifactory username with publish permission on `npm-local`.
- `ARTIFACTORY_PASS` — corresponding password.

### Maintainer permissions

Maintainers cutting releases need:

- **Write access** to the repository (to merge PRs and create releases via the UI).
- **Release create** permission (default for collaborators with write access).

No local tooling is required for the recommended flow. GitHub CLI (`gh`) and other local scripts are only needed for the fallback path; see [Local Tooling Reference](#local-tooling-reference).

## Pre-Release Checklist

Before cutting a release, confirm:

1. **Version bump merged**: a PR that updates `cpp-lib/version.txt`, `pitaya-sharp/NPitaya/package.json`, `pitaya-sharp/NPitaya-csproj/NPitaya.csproj`, and `unity/NPitaya.nuspec` to the target version is merged into the release target branch (`master` for stable; the feature branch for prereleases). The `update-version.sh` helper updates all four files in one shot — run it locally inside the version-bump PR branch and commit the changes.
2. **Tag is unused**: confirm the tag does not exist on https://github.com/topfreegames/libpitaya-cluster/tags.
3. **Version respects semver ordering**: see [Versioning Conventions](#versioning-conventions).
4. **CHANGELOG.md updated** (stable releases only): move `[Unreleased]` items into a new `[X.Y.Z]` section, in the same PR as the version bump.
5. **Repo secrets exist**: see [Prerequisites](#prerequisites).
6. **Latest CI on the target branch is green**: confirm the version-bump PR merge built successfully.

## Recommended Release Flow

All routine releases go through GitHub's Releases UI. This creates the tag on the chosen branch, generates release notes, and triggers `build-and-release.yml`.

1. **Open the release page**: https://github.com/topfreegames/libpitaya-cluster/releases/new

2. **Choose tag**: type the new tag (e.g., `v1.2.0`, `v1.3.0-rc.1`, `v1.4.0-alpha.1`). GitHub will offer to create the tag — accept.

3. **Choose target branch**:
   - `master` for stable releases (`vX.Y.Z`).
   - The feature branch for prereleases. The published artifact reflects the tagged commit.

4. **Title**: `Release vX.Y.Z`.

5. **Description**: click *Generate release notes*. Edit to highlight breaking changes or migration notes if relevant.

6. **Pre-release flag**: tick this box for any tag with `-alpha`, `-beta`, or `-rc` suffix.

7. **Publish release**. GitHub creates the tag remotely; the tag push triggers `build-and-release.yml`.

8. Proceed to [Validation](#validation).

## What the CI Pipeline Does

`.github/workflows/build-and-release.yml` runs on tag push and contains these jobs:

1. **`cache-check`** — looks for cached consolidated libraries keyed by the SHA of `cpp-lib/` and `vendor/`. On hit, the build matrix is skipped.
2. **`build`** (5-platform matrix, parallel, cache-miss only) — Linux x86_64, Linux ARMv8, macOS x86_64, macOS ARM64, Windows x86_64. Uses Conan and CMake.
3. **`consolidate`** — downloads platform artifacts and assembles the `runtimes/` tree.
4. **`package`** — runs `package.sh VERSION=<tag>`. Strips the `v` prefix, copies sources and native libs into `package/`, and overwrites `package/package.json` with the tag version.
5. **`publish`** — `npm publish` to `https://artifactory.tfgco.com/artifactory/api/npm/npm-local/` using `ARTIFACTORY_USER` / `ARTIFACTORY_PASS`.

A typical run takes ~30 minutes on a cache miss and ~5 minutes on a cache hit.

> The published version is determined by the **tag name**, not by the version files in the repo at the tagged commit. `package.sh` rewrites `package.json` from the tag before publishing. The version files still need to be in sync (see [Pre-Release Checklist](#pre-release-checklist)) so consumers reading the source see the right version, and so locally-built NuGet artifacts are correct.

## Validation

After publishing the release on GitHub, validate end-to-end. Skipping validation is the most common cause of broken releases reaching consumers.

### 1. Confirm the workflow ran

Visit https://github.com/topfreegames/libpitaya-cluster/actions/workflows/build-and-release.yml and confirm a run for the new tag is in progress or completed.

All required jobs must succeed: `cache-check`, `build` (cache miss only), `consolidate`, `package`, `publish`.

If no run started within ~30 seconds of clicking *Publish release*, the tag was likely created without triggering the workflow (rare GitHub timing). Delete the release and recreate it.

### 2. Confirm the artifact on Artifactory

Browse https://artifactory.tfgco.com → repositories → `npm-local` → `com.wildlifestudios.npitaya`. Confirm the new version `X.Y.Z` is listed and the tarball is downloadable.

### 3. Confirm OpenUPM

Visit https://openupm.com/packages/com.wildlifestudios.npitaya/. The new version should appear in the version list within ~10 minutes after CI succeeds. If it does not appear after ~30 minutes, click the OpenUPM build status link on that page and check for ingestion errors.

### 4. Smoke-test in a Unity project (stable releases)

In a fresh Unity project, add or update the package and confirm:

- `Packages/manifest.json` resolves `com.wildlifestudios.npitaya@X.Y.Z`.
- `Runtime/Plugins/runtimes/<platform>/libpitaya_cpp.{so,dylib,dll}` is present in the imported package on each target platform.
- A simple `PitayaCluster.Configure()` call in Play Mode loads without `DllNotFoundException`.

For prereleases this step is optional but recommended before promoting downstream.

## Package Structure

The final package follows the Unity package format:

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

## Caching

The workflow uses two caches to keep release builds fast:

- **Conan cache**: per-platform build dependencies.
- **Consolidated cache**: processed native libraries across all platforms, keyed by the contents of `cpp-lib/` and `vendor/`.

Cache hits skip the entire build matrix and proceed directly to packaging and publishing. Cache misses run the full build for all platforms and write a fresh entry for subsequent releases.

## Local Tooling Reference

The following commands are useful for the version-bump PR or as fallbacks when the recommended UI-driven flow is unavailable. **None of them are part of the routine release flow.**

### `update-version.sh`

Updates all four version files in one shot. Use it inside the version-bump PR branch — not as part of cutting a release:

```bash
VERSION=1.2.0 ./update-version.sh
```

The script accepts versions with or without the `v` prefix. If changes are detected it commits with `chore: bump version to vX.Y.Z` and pushes to the current branch — within a normal PR workflow this means the bump commit lands on your PR branch.

Files updated:
- `cpp-lib/version.txt`
- `pitaya-sharp/NPitaya/package.json`
- `pitaya-sharp/NPitaya-csproj/NPitaya.csproj`
- `unity/NPitaya.nuspec`

### `package.sh`

Used by CI; not normally run locally. If you need to reproduce the packaging step locally to debug a CI failure, place pre-built native libraries in `downloaded-artifacts/` and run:

```bash
VERSION=1.2.0 ./package.sh
```

### `make release` — fallback only

`make release VERSION=vX.Y.Z [PRERELEASE=true]` runs `update-version.sh`, then calls `gh release create` to publish the release. It is provided for emergencies — for example, when the GitHub Releases UI is unavailable. **Do not use it for routine releases**:

- It commits and pushes the version bump directly to the current branch, bypassing PR review.
- It cannot be undone safely once it has triggered CI.
- The CI side-effects are identical to the UI flow, so there is no functional advantage to running it locally.

### NuGet packaging (`make nuget-pack` / `make nuget-push`)

The CI pipeline does not publish NuGet — this is a known gap. The Makefile provides `nuget-pack` (builds `NugetOutput/NPitaya.X.Y.Z.nupkg` from `unity/NPitaya.nuspec`) and `nuget-push NUGET_API_KEY=<key>` (pushes to nuget.org). These are local-only and follow the same caveats as `make release`. The recommended fix is to extend `build-and-release.yml` with a NuGet publish step; track this in an issue rather than relying on routine local pushes.

## Troubleshooting

### Workflow did not trigger

If you published a release but no run started:

1. Confirm the tag exists at https://github.com/topfreegames/libpitaya-cluster/tags.
2. Delete the release and recreate it.
3. If still nothing, file an issue — there may be a workflow trigger configuration regression.

### Build failures

Inspect the GitHub Actions logs for the failing job. Common causes:

- Conan dependency resolution failed — often a transient network issue; retry the run.
- Native library mismatch on a specific platform — fix in `cpp-lib/` and cut a new patch version.

### Publish failures

If the `publish` job fails with auth errors:

1. Confirm `ARTIFACTORY_USER` and `ARTIFACTORY_PASS` are set in repo secrets.
2. Verify the credentials still have publish permission on `npm-local`.
3. Confirm the version does not already exist on Artifactory — npm refuses to overwrite an existing version.

### Rolling back a release

Once a tarball is on Artifactory, **delete-and-republish is unsafe**. Package managers and OpenUPM may have cached the bad version. Prefer **rolling forward with a new patch release** (`vX.Y.Z+1`) cut via the recommended UI flow.

If you must remove a release that should never have been cut (wrong target branch, semver-broken version), open a maintainer issue and coordinate with a release owner. Do not delete tags or releases unilaterally — releases that have triggered CI may already be live on Artifactory and OpenUPM, and ad-hoc deletion creates more inconsistency, not less.

## Artifactory Configuration

The Unity/npm package is published to:

- **Registry**: `https://artifactory.tfgco.com/artifactory/api/npm/npm-local`
- **Scope**: `@wls`
- **Package name**: `com.wildlifestudios.npitaya`

## CHANGELOG

Release notes are tracked in `CHANGELOG.md`. As part of the version-bump PR for a stable release, move `[Unreleased]` items into a new `[X.Y.Z]` section. The auto-generated GitHub release notes complement, but do not replace, the CHANGELOG.
