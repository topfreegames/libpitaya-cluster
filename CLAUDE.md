# CLAUDE.md

Guidance for Claude Code when working in this repo.

## What this project is

`libpitaya-cluster` is a multi-language port of the Go-based [Pitaya](https://github.com/topfreegames/pitaya) game-server framework. The C++ core (`cpp-lib`) is built once and consumed via thin wrappers in C# (`pitaya-sharp/NPitaya`) and Python (`python-lib`). A Go server example (`go-server`) and a Unity project (`unity/`) demonstrate cross-language interop.

## Repo layout

| Path | Purpose |
| --- | --- |
| `cpp-lib/` | C++ core library (Conan 2 + CMake). Source of truth for native binaries. |
| `pitaya-sharp/` | C# solution. `NPitaya` is the main wrapper consumed by Unity / .NET servers. |
| `python-lib/` | Python wrapper (`pitayaserver` package) over the C++ shared library. |
| `python-example/` | Example Python pitaya server. |
| `go-server/` | Example Go pitaya server using the upstream Go Pitaya. |
| `unity/` | Unity project that uses NPitaya. |
| `integration-test/` | End-to-end lame-duck-mode test harness (C++ + Docker). |
| `pitaya-protos/` | **Submodule** — shared protobuf definitions (used by all languages). |
| `precompiled/` | Symlinks to native binaries under `pitaya-sharp/NPitaya/Runtime/Plugins/`. |
| `docs/` | Release process, Ubuntu 22.04 compatibility notes, memory-bank. |
| `vendor/` | Go vendored deps. |

## Critical gotchas

- **Submodules are required.** Run `git submodule update --init --recursive` after cloning. Without `pitaya-protos/`, nothing builds.
- **macOS toolchain is pinned to Clang 15 (Xcode 14.3).** gRPC 1.54.3 does not compile on Clang 17 / Xcode 16+. The `cpp-lib/Makefile` enforces this via Conan profile flags (`-s compiler.version=15`). Don't try to "upgrade" the toolchain without also upgrading gRPC.
- **No Windows support** for the C++ build (explicitly excluded in `cpp-lib/conanfile.py`).
- **Native binaries are gitignored — never commit them.** `.gitignore` excludes `libpitaya_cpp.{so,dylib,dll,bundle}` under `pitaya-sharp/NPitaya/Runtime/Plugins/runtimes/**` and `pitaya-sharp/NPitaya-csproj/runtimes/**`. CI builds and publishes them on release.
- **The native binary is `libpitaya_cpp`, not `libpitaya_cluster`.** Old docs may use the latter name — it's wrong.
- **Protos must be compiled before building any language wrapper.** Run `make protos-compile` from the repo root. Generated files land under `cpp-lib/include/pitaya/protos/`, `pitaya-sharp/NPitaya/src/gen/`, `unity/Assets/Gen/`, and `python-lib/pitayaserver/gen/`. Do not hand-edit anything in those `gen/` directories.
- **Known broken root Make targets**: `make build-all` references `build-csharp-lib-release` and `build-cpp-unity`, neither of which is defined. Use the per-component Makefiles instead until that's fixed.

## Common commands

### Local dev dependencies (NATS + etcd)
```bash
make start-deps   # docker-compose up -d
make stop-deps
```

### Protobuf
```bash
make protos-compile        # all languages
make protos-compile-cpp    # C++ only (also runs `git submodule update`)
```

### C++ core (`cpp-lib/`)
```bash
cd cpp-lib
make build-mac-release       # macOS x86_64 + arm64
make build-mac-unity         # macOS, with macosx_bundle=True (for Unity)
make build-linux-release     # Linux native
make build-docker-image && make build-linux-docker   # Linux build via Ubuntu 22.04 container (GLIBC 2.35)
make build-mac-debug && make run-mac-debug           # tests + coverage
```
Build outputs live in `cpp-lib/_builds/<platform>-<arch>-...-<config>/`. The root `make build-cpp-on-mac` lipos the per-arch dylibs and copies them into `pitaya-sharp/NPitaya/Runtime/Plugins/`.

### C# (`pitaya-sharp/`)
```bash
cd pitaya-sharp
make build    # dotnet build NPitaya-csproj --configuration Release
make test     # dotnet test NPitaya.Tests (needs `make start-deps` running)
```

### Python (`python-lib/`)
```bash
cd python-lib && pip install -e .
# Then run the example:
cd python-example && python example.py
```

### Go server example
```bash
make build-go-server   # go mod tidy
make run-go-server
```

### Integration tests
```bash
cd integration-test
make build
make test-lame-duck   # full end-to-end with NATS cluster, validates ≥95% RPC/event success
```

## Release

Releases are automated via `make release VERSION=vX.Y.Z`. See [docs/RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md) for the full flow (GitHub Actions → Artifactory → OpenUPM-equivalent registry). Version source of truth: `cpp-lib/version.txt`.

**Prefer the automated `make release` flow.** Manual releases (hand-editing version files, tagging, and triggering the workflow) are documented in `docs/RELEASE_PROCESS.md` as a fallback only — they're error-prone and skip the version-bump and changelog steps that `make release` handles. Don't propose them unless the automated flow is broken.

## Style / convention notes

- Don't reformat unrelated code. Match the surrounding style.
- Don't add backwards-compatibility shims unless explicitly asked — the API is documented as unstable.
- User-facing changes need a `CHANGELOG.md` entry under `## [Unreleased]`. Group by `Added / Changed / Deprecated / Removed / Fixed / Security`. Call out which language interface(s) the change affects, since a "major" bump may break only one language.
- See [CONTRIBUTING.md](CONTRIBUTING.md) for contributor-facing guidelines.

## Useful docs in-tree

- [docs/RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md) — release automation, version bumps, Artifactory.
- [docs/UBUNTU_22_04_COMPATIBILITY.md](docs/UBUNTU_22_04_COMPATIBILITY.md) — Linux build / GLIBC notes.
- [cpp-lib/docs/LAME_DUCK_MODE.md](cpp-lib/docs/LAME_DUCK_MODE.md) — NATS graceful-shutdown behavior.
- [integration-test/](integration-test/) — has its own README describing the lame-duck test harness.
