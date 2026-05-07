# Contributing to libpitaya-cluster

Thanks for your interest in contributing! This document outlines the basic guidelines for contributing to the project.

## Getting Started

1. Fork the repository and clone your fork.
2. Initialize submodules: `git submodule update --init --recursive`.
3. Make sure you can build the components you plan to change. See the [README](README.md) and per-component documentation:
   - [cpp-lib](cpp-lib) — the C++ core library
   - [pitaya-sharp](pitaya-sharp) — the C# wrapper (NPitaya)
   - [python-lib](python-lib) — the Python wrapper

## Development Workflow

1. Create a branch from `master` for your change.
2. Keep changes focused — one logical change per pull request.
3. Match the surrounding code style. Don't reformat unrelated code in the same PR.
4. Run the relevant tests and examples locally before opening a PR:
   - Start the test dependencies (etcd, NATS) with `make start-deps`.
   - Run the Go example server with `make run-go-server`.
   - Build the C++ library and run its tests via the targets in `cpp-lib/`.
   - Build the C# library with `make build-csharp-lib-release` and run its tests.
5. Update documentation when you change behavior, configuration, or public APIs.

## Commit Messages

- Use clear, descriptive commit messages.
- Prefer the conventional-commit style already used in the history (`feat:`, `fix:`, `chore:`, `docs:`, etc.).
- Reference issues or PRs when relevant (e.g. `Fixes #123`).

## Pull Requests

- Open the PR against `master`.
- Describe the motivation, the change, and how you tested it.
- Make sure CI passes. If a check fails, investigate and fix the underlying issue rather than retrying blindly.
- Be responsive to review feedback. Push follow-up commits rather than force-pushing while review is in progress.

## Changelog

Any user-facing change should be reflected in [CHANGELOG.md](CHANGELOG.md) as part of the same PR. The file follows the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format.

- Add your entry under the `## [Unreleased]` section. The maintainer renames it to the new version on release.
- Group entries by type: `Added`, `Changed`, `Deprecated`, `Removed`, `Fixed`, `Security`.
- Write entries from the user's perspective — what changed for someone consuming the library, not the implementation detail.
- Call out breaking changes explicitly, including which language interfaces are affected. Because this library is consumed from C++, C#, and Python, a bump that breaks one language's interface may leave the others untouched — be specific.
- Skip the changelog for purely internal changes (refactors with no user-facing effect, CI tweaks, doc-only updates).

## Reporting Issues

When opening an issue, please include:

- A clear description of the problem or proposal.
- Steps to reproduce, expected behavior, and actual behavior (for bugs).
- The component affected (cpp-lib, pitaya-sharp, python-lib, etc.) and your environment (OS, compiler, runtime versions).
- Relevant logs or stack traces.

## Releasing

Releases are handled by maintainers. See [docs/RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md) for the full process.

## Code of Conduct

Be respectful and constructive. Assume good intent, give actionable feedback, and keep discussions focused on the work.
