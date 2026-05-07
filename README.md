# [WIP] pitaya-server

## Overview

The original [Pitaya](https://github.com/topfreegames/pitaya) project supports building Pitaya servers in Go. This project provides the same functionality for other languages. Currently, the supported languages are **C++**, **C#**, and **Python**, plus a Unity integration via the C# wrapper.

> **Note**: This library is in early stage and the API is not stable. Bugs are expected, and breaking changes may land between minor versions. Read [CHANGELOG.md](CHANGELOG.md) before upgrading.

## Requirements

- **Conan** ≥ 2.x and **CMake** ≥ 3.26 — for building the C++ core.
- **Docker** + **docker-compose** — to run NATS and etcd locally.
- **.NET SDK** (and Mono on macOS, if needed) — for the C# library and examples.
- **Python 3** — for the Python wrapper and example.
- **Go** — only required if you want to run the Go example server.
- **macOS only**: Xcode **14.3 / Clang 15**. gRPC 1.54.3 does not compile on Clang 17 (Xcode 16+); the `cpp-lib` Makefile pins the Conan profile to `compiler.version=15` to enforce this. Older Xcodes are available at https://developer.apple.com/download/all/.
- **Windows is not supported** for the C++ build (explicitly excluded in `cpp-lib/conanfile.py`).

## Pitaya dev dependencies (NATS + etcd)

The `go-server`, C# examples, and the Unity integration all expect an etcd instance on `localhost:2379` and a NATS instance on `localhost:4222`. The repo ships a `docker-compose.yml` that runs both:

```bash
make start-deps   # docker-compose up -d
make stop-deps    # docker-compose down
```

## Components

- **[cpp-lib](cpp-lib)** — the C++ core library. Source of truth for the native binary (`libpitaya_cpp.{so,dylib,dll,bundle}`).
- **[pitaya-sharp](pitaya-sharp)** — C# solution. The main project is `NPitaya`, a library that wraps the native C++ library and provides a convenient interface for writing Pitaya servers in C# (and consuming from Unity).
- **[python-lib](python-lib)** — Python wrapper (`pitayaserver` package) over the C++ shared library. Used by `python-example/`.
- **[go-server](go-server)** — example Pitaya server in Go (uses upstream Go Pitaya). Run with `make run-go-server`.
- **[unity](unity)** — Unity project that consumes `NPitaya`. Native libraries land in `pitaya-sharp/NPitaya/Runtime/Plugins/` after a C++ build; Unity picks them up via the package layout.
- **[integration-test](integration-test)** — end-to-end lame-duck-mode test harness (C++ + Docker, multi-node NATS cluster).

## Installation

| Language | Project location                  |
| -------- | --------------------------------- |
| C++      | [cpp-lib](cpp-lib)                |
| C#       | [pitaya-sharp](pitaya-sharp)      |
| Python   | [python-lib](python-lib)          |

## Building

Each component has its own Makefile. The most common targets:

### C++ core
```bash
cd cpp-lib
make build-mac-release       # macOS x86_64 + arm64
make build-mac-unity         # macOS, with macosx_bundle=True (for Unity)
make build-linux-release     # Linux native
make build-docker-image && make build-linux-docker   # Linux build via Ubuntu 22.04 container
```
Outputs go to `cpp-lib/_builds/<platform>-<arch>-...-<config>/libpitaya_cpp.{dylib,so,bundle}`. The root `make build-cpp-on-mac` lipos per-arch dylibs and copies them into `pitaya-sharp/NPitaya/Runtime/Plugins/`.

### C# (NPitaya)
```bash
cd pitaya-sharp
make build   # dotnet build NPitaya-csproj --configuration Release
make test    # dotnet test NPitaya.Tests (needs `make start-deps` running)
```

### Python
```bash
cd python-lib && pip install -e .
cd ../python-example && python example.py
```

### Protobuf
The shared protos live in the `pitaya-protos` submodule. Run `git submodule update --init --recursive` after cloning, then `make protos-compile` from the repo root to regenerate code for all languages.

## Documentation

- **[CONTRIBUTING.md](CONTRIBUTING.md)** — contribution guidelines, dev workflow, changelog format.
- **[CHANGELOG.md](CHANGELOG.md)** — release notes; new entries go under `## [Unreleased]`.
- **[docs/RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md)** — automated release flow (`make release`), versioning, Artifactory publishing.
- **[docs/UBUNTU_22_04_COMPATIBILITY.md](docs/UBUNTU_22_04_COMPATIBILITY.md)** — Linux build / GLIBC notes.
- **[cpp-lib/docs/LAME_DUCK_MODE.md](cpp-lib/docs/LAME_DUCK_MODE.md)** — how the NATS client handles graceful server shutdowns, including message buffering, thread-safe operations, and reconnection strategies.
- **[integration-test/](integration-test)** — README describing the lame-duck integration test harness.
- Per-component READMEs under [cpp-lib/](cpp-lib), [python-lib/](python-lib), and [python-example/](python-example).

## Releasing a new version

See [docs/RELEASE_PROCESS.md](docs/RELEASE_PROCESS.md) for the full release process, including the automated `make release` workflow, version management, and Artifactory publishing details.

> Do NOT commit native binaries — they are gitignored. Build them locally via the `cpp-lib` Make targets (see [cpp-lib](cpp-lib)).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.
