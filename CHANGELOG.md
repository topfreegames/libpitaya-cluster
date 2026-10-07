# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
This project follows loosely the [semantic versioning scheme](https://semver.org/spec/v2.0.0.html).
The project differs in that major bumps in the version number do not necessarily break all interfaces.
Since this library is consumed from different programming languages, bumping a major version
may not necessarily break the interface for every language. Read the release notes for each
version to see which interfaces changed.

## [Unreleased]

## [1.2.1-rc.1] - 2026-05-14

### Fixed

- Restore `libpitaya_cpp` inclusion in Unity 2021/2022 builds. The PluginImporter `serializedVersion 3` layout introduced in 1.0.11 does not filter the per-architecture natives correctly on pre-Unity 6: `linux-x86_64`/`linux-armv8` collide on the Linux64 slot, `macos-x86_64`/`macos-arm64` collide on OSXUniversal, Unity silently excludes them all, and the build then crashes at runtime with `DllNotFoundException`. A new build-output post-process (`NPitayaPluginConfigurator`) now stamps the correct native into the player's plugins directory after every build on Unity &lt; 6000, replacing any wrong-arch variant Unity may have copied and creating the file if Unity dropped them all. Source assets and `.meta` files are never touched. Unity 6 keeps using the authored `.meta` files.

## [1.2.0] - 2026-05-05

### Added

- `ReportDistribution` added to `IMetricsReporter` to support distribution-type metrics.

## [1.1.0] - 2025-12-15

### Added

- NATS lame duck mode handling: clients now react to NATS server lame duck notifications and gracefully drain instead of being abruptly disconnected ([#71](https://github.com/topfreegames/libpitaya-cluster/pull/71)).

## [1.0.13] - 2025-12-10

### Added

- Unity IL2CPP support for builds that compile NPitaya with the IL2CPP scripting backend ([#75](https://github.com/topfreegames/libpitaya-cluster/pull/75)).

### Fixed

- `libpitaya_cpp` now loads correctly in the Unity Editor on Windows ([#73](https://github.com/topfreegames/libpitaya-cluster/pull/73)).

### Changed

- Bumped GitHub Actions macOS runners from `macos-13` to `macos-14` ([#76](https://github.com/topfreegames/libpitaya-cluster/pull/76)).

## [1.0.12] - 2025-12-02

### Added

- Send `SIGTERM` on initialization failure so process supervisors can react to a failed boot ([#74](https://github.com/topfreegames/libpitaya-cluster/pull/74)).

## [1.0.11] - 2025-08-26

### Fixed

- `pitaya-sharp` meta files updated to only set the standalone platform, fixing native plugin selection for non-standalone Unity targets ([#72](https://github.com/topfreegames/libpitaya-cluster/pull/72)).

## [1.0.10] - 2025-07-30

### Changed

- Refactored the NATS Conan recipe to target NATS 3.10 ([#70](https://github.com/topfreegames/libpitaya-cluster/pull/70)).

## [1.0.9] - 2025-07-28

### Fixed

- Tuned NATS reconnect parameters to better tolerate transient broker outages ([#69](https://github.com/topfreegames/libpitaya-cluster/pull/69)).

## [1.0.8] - 2025-07-24

### Fixed

- Deduplicated macOS `dylib` architectures in the Unity meta files ([#68](https://github.com/topfreegames/libpitaya-cluster/pull/68)).

## [1.0.7] - 2025-07-22

### Fixed

- Unity meta files now point at the correct native library for each platform, fixing preloading on macOS and Linux ([#67](https://github.com/topfreegames/libpitaya-cluster/pull/67)).

## [1.0.6] - 2025-07-21

### Added

- CI publish step that builds and uploads Unity/npm artifacts to Artifactory on tag push ([#66](https://github.com/topfreegames/libpitaya-cluster/pull/66)).

### Changed

- Linux build switched to a glibc 2.35 baseline for compatibility with Ubuntu 22.04 ([#66](https://github.com/topfreegames/libpitaya-cluster/pull/66)).

## [1.0.5] - 2025-02-21

### Fixed

- Prevent destruction of in-flight promises that caused crashes in some Unity environments ([#65](https://github.com/topfreegames/libpitaya-cluster/pull/65)).

## [1.0.4] - 2024-09-09

### Fixed

- Do not send `SIGTERM` when the process is already shutting down, avoiding spurious termination signals during normal shutdown.

## [1.0.3] - 2024-08-23

### Fixed

- libpitaya-cluster now exits when it cannot reconnect to NATS after exhausting retries, instead of staying alive in an unrecoverable state ([#63](https://github.com/topfreegames/libpitaya-cluster/pull/63)).

## [1.0.2] - 2024-07-09

### Changed

- Migrated to a proper ETCD client with gRPC timeouts; reset retry counter on success and added defaults for the retry configuration ([#60](https://github.com/topfreegames/libpitaya-cluster/pull/60)).
- Bumped `nats.c` from 3.3.0 to 3.8.0 ([#61](https://github.com/topfreegames/libpitaya-cluster/pull/61)).

### Fixed

- ETCD keepalive and watcher reliability fixes ([#62](https://github.com/topfreegames/libpitaya-cluster/pull/62)).
- Packaging: missing `dylib` and incorrect symlinks in the published artifact.

## [1.0.1] - 2024-02-07

### Changed

- Removed manually vendored `protobuf` and `System.Runtime.CompilerServices.Unsafe`; both are now consumed from NuGet.
- CI: consolidated prebuilt native libs and added an ARMv8 macOS build target.

## [1.0.0 / v0.15.0] - 2023-09-17

### Changed

- Repository refactored to publish NPitaya as a Unity Package Manager (UPM) package; C# project structure reorganized.
- CI matrix build for native libraries (Linux x86_64, macOS x86_64/arm64, Windows x86_64).

## [0.14.3] - 2022-03-30

### Fixed

- NATS initial connect retry now runs synchronously and surfaces the underlying error in the initialization failure ([#44](https://github.com/topfreegames/libpitaya-cluster/pull/44)).
- Test suite build restored ([#43](https://github.com/topfreegames/libpitaya-cluster/pull/43)).
- Left-shift-by-minus-one bug on the first ETCD retry.

### Changed

- ETCD retry values hardcoded to avoid a breaking change introduced in 0.14.2.
- Bumped OpenSSL; reverted moving some dependencies into Conan.
- More detailed RPC call errors.

## [0.14.2] - 2022-03-23

### Fixed

- ETCD exponential backoff retry bug (left shift by `-1`).

### Changed

- Bumped boost 1.75.0 → 1.78.0, openssl 1.1.1l → 1.1.1m, nats.c 2.5.0 → 3.3.0, gRPC 1.37.0 → 1.44.0, protobuf 3.15.5 → 3.19.2.
- Moved `cpprestsdk` and gRPC into Conan instead of vendoring them in the repo (fixes compilation with Xcode 13.1).
- C++ standard bumped from C++11 to C++17.
- Docker build image upgraded from Clang 11 to Clang 13.
- Recompiled protos with the updated `protoc`.

## [0.14.1] - 2022-02-17

### Changed

- Replaced the custom `Pitaya::NatsStatus` with the native client's `natsStatus` for status codes and error messages, improving NATS error reporting.
- Added `--build=missing` flag to Conan on macOS debug builds.

## [0.14.0] - 2021-10-06

### Added

- NATS reconnection buffer size config option exposed in the C++ layer ([#34](https://github.com/topfreegames/libpitaya-cluster/pull/34)).

## [0.13.0] - 2021-08-30

### Changed

- Reverted a breaking change in NPitaya method visibility introduced in 0.12.0 ([#33](https://github.com/topfreegames/libpitaya-cluster/pull/33)).

## [0.12.0] - 2021-08-12

### Added

- `PitayaTimeoutException` and `PitayaRouteNotFoundException` in NPitaya ([#30](https://github.com/topfreegames/libpitaya-cluster/pull/30)).
- `retryDelayMilliseconds` field on `EtcdServiceDiscoveryConfig` to configure the ETCD retry base delay ([#27](https://github.com/topfreegames/libpitaya-cluster/pull/27)).

### Changed

- ETCD connection retry now uses exponential backoff ([#27](https://github.com/topfreegames/libpitaya-cluster/pull/27)).
- RPC calls return more specific exceptions (`PitayaTimeoutException`, `PitayaRouteNotFoundException`) instead of generic errors ([#30](https://github.com/topfreegames/libpitaya-cluster/pull/30)).
- NATS connection now retries on first connect ([#28](https://github.com/topfreegames/libpitaya-cluster/pull/28)).

### Breaking

- `SDConfig` constructor gained a `retryDelayMilliseconds` argument ([#27](https://github.com/topfreegames/libpitaya-cluster/pull/27)).

## [0.11.0] - 2019-10-10

### Added

- `NPITAYA_DEBUG` compile-time define that prints additional diagnostic information.

## [0.10.0] - 2019-10-04

### Added

- Optional `serverTypeFilters` array passed to the ETCD worker to limit which server types are watched.

### Fixed

- Datadog summary metric reported as a timer instead of a distribution.
- Incorrect function parameters in NPitaya bindings.

## [0.9.1] - 2019-08-12

### Added

- Handler `response_time_ns` reporting in NPitaya.

## [0.9.0] - 2019-08-02

### Changed

- `PitayaSession` now returns Tasks for asynchronous operations.

## [0.8.1] - 2019-08-01

### Fixed

- Compilation issues on certain toolchains.

## [0.8.0] - 2019-08-01

### Added

- Asynchronous gRPC server and async RPC dispatch using a fixed-size thread pool.
- `SendPushToUser` and `SendKickToUser` use the async thread pool as well.
- Custom metrics support and metrics tests.

## [0.6.8] - 2019-06-17

### Changed

- Removed unused `_connClosed` flag and added more verbose logging around the NATS connection lifecycle.

## [0.6.7] - 2019-06-14

### Added

- Debug build target for the Unity native library.

### Fixed

- NATS client shutdown sequence.
- Compilation of the Unity test script.

## [0.6.6] - 2019-06-14

### Fixed

- Race conditions in tests: semaphore handling fixed and timing assertions tightened.

## [0.6.5] - 2019-06-10

### Added

- `NPitaya.asmdef` so the C# bindings work with Unity assembly definitions.
- Reconnection-to-ETCD test coverage in service discovery.

### Changed

- Pinned protobuf back to 3.7.0 (3.7.1 introduced incompatibilities).
- Pass logger through to `GrpcClient`.
- More verbose logging on failed RPCs.

### Fixed

- Servers ticker now restarts after an ETCD reconnect.
- ETCD watcher now restarts after reconnect.
- Lock `_serversById` while modifying it to avoid concurrent-access bugs.

## [0.6.1] - 2019-05-24

### Fixed

- `maxNumberOfRetries` was not being passed from the C# layer down to the C++ core.

### Changed

- Removed dead code and added more logging.

## [0.6.0] - 2019-05-16

### Added

- Initial NPitaya C# bindings published alongside the C++ library.
- gRPC client RPC timeout option.
- Utility function to name threads (not implemented on Windows yet).

### Fixed

- Null-listener handling.
- Renamed protos namespace to avoid conflicts with `pitaya`.
- Test binary compilation.

## [0.7.0] - 2019-05-25

### Added

- StatsD metrics reporter.

> Note: tag `0.7.0` was cut between `0.6.8` and `0.7.0` proper; the version numbering crossed paths during a transitional period in May–June 2019.

## [0.5.3] - 2019-03-22

### Added

- More service discovery tests, ParseServer test coverage, log tags to avoid typos.
- Debug Mac target.
- Test coverage report for test builds.

### Changed

- Stricter `ParseServer` validation.
- Refactored test/build script for robustness; tests now run as part of the script.

### Fixed

- Compilation of the example binary.
- Formatting issues.

## [0.5.2] - 2019-03-21

### Added

- Tests for `ParseEtcdKey`.

### Fixed

- Typo in the logger name.
- Test compilation.

## [0.5.1] - 2019-03-20

### Changed

- Bumped gRPC dependency to 1.19.1.

### Fixed

- Crash on startup in some configurations.

## [0.5.0] - 2019-03-20

### Added

- `GrpcConfig` and bindings to the new `c_wrapper` API.
- Initial public release of the C++ cluster library with ETCD service discovery and NATS RPC transport.

### Fixed

- Race condition in cluster initialization.

## [0.4.0] - 2019-03-18

### Added

- Option to initialize the cluster with either gRPC or NATS as the RPC transport.
- Service discovery test scaffolding; mock now allows controlling `onWatch`.
- Public headers exposed for downstream consumers.
- Option to pass an `EtcdClient` to service discovery.
- Ticker tests.

### Changed

- Large refactor of the C++ core; reduced namespace nesting.
- Removed `onWatch` from the service discovery constructor.
- C# binding and example updated for gRPC.
- `pack-only` target now clears the NuGet output directory.

### Fixed

- Undefined `std::function` reference on some toolchains.

## [0.3.10] - 2019-03-13

### Added

- Working Python library for sending and receiving RPCs; pip package published.
- Working pitaya example.
- Generated files for gRPC.

### Changed

- Made some internal methods private.
- Python lib linted and refactored.
- Build supports linking against `libstdc++` on Linux.

### Fixed

- Memory leak when returning errors from the Python and C# libraries.
- Small C# library fix.

## [0.3.0] - 2019-04-30

### Added

- `maxNumberOfRetries` option on the service discovery configuration.
- `.nuspec` file for NuGet packaging.

### Changed

- RPC server now starts before service discovery.
- Removed unused lease keep-alive from binding storage.
- Removed `connectionTimeout` from `grpcConfig` (functionality already removed upstream).

> Note: tag `0.3.0` was cut **after** the `0.4.x` and `0.5.x` lines (April 30, 2019) as a follow-up release on an older branch. Version numbers are not strictly chronological in this period.

## [0.2.2] - 2018-08-28

### Added

- Initial gRPC support alongside NATS.
- Jaeger tracing support.
- Customization for registering remotes.
- Unity example project.
- C# project files (`.sln`, `.csproj`).

### Changed

- Cluster modules split into separate creation steps.
- Library refactored.
- Removed hack used to process NATS RPCs.
- Switched C# projects to MonoDevelop tooling.

[Unreleased]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.2.1-rc.1...HEAD
[1.2.1-rc.1]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.2.0...v1.2.1-rc.1
[1.2.0]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.13...v1.1.0
[1.0.13]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.12...v1.0.13
[1.0.12]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.11...v1.0.12
[1.0.11]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.10...v1.0.11
[1.0.10]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.9...v1.0.10
[1.0.9]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.8...v1.0.9
[1.0.8]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.7...v1.0.8
[1.0.7]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.6...v1.0.7
[1.0.6]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.5...v1.0.6
[1.0.5]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/topfreegames/libpitaya-cluster/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/topfreegames/libpitaya-cluster/compare/v0.15.0...v1.0.1
[1.0.0 / v0.15.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.14.3...v0.15.0
[0.14.3]: https://github.com/topfreegames/libpitaya-cluster/compare/0.14.2...0.14.3
[0.14.2]: https://github.com/topfreegames/libpitaya-cluster/compare/0.14.1...0.14.2
[0.14.1]: https://github.com/topfreegames/libpitaya-cluster/compare/0.14.0...0.14.1
[0.14.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.13.0...0.14.0
[0.13.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.12.0...0.13.0
[0.12.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.11.0...0.12.0
[0.11.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.10.0...0.11.0
[0.10.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.9.1...0.10.0
[0.9.1]: https://github.com/topfreegames/libpitaya-cluster/compare/0.9.0...0.9.1
[0.9.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.8.1...0.9.0
[0.8.1]: https://github.com/topfreegames/libpitaya-cluster/compare/0.8.0...0.8.1
[0.8.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.7.0...0.8.0
[0.7.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.8...0.7.0
[0.6.8]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.7...0.6.8
[0.6.7]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.6...0.6.7
[0.6.6]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.5...0.6.6
[0.6.5]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.1...0.6.5
[0.6.1]: https://github.com/topfreegames/libpitaya-cluster/compare/0.6.0...0.6.1
[0.6.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.5.3...0.6.0
[0.5.3]: https://github.com/topfreegames/libpitaya-cluster/compare/0.5.2...0.5.3
[0.5.2]: https://github.com/topfreegames/libpitaya-cluster/compare/0.5.1...0.5.2
[0.5.1]: https://github.com/topfreegames/libpitaya-cluster/compare/0.5.0...0.5.1
[0.5.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.4.0...0.5.0
[0.4.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.3.10...0.4.0
[0.3.10]: https://github.com/topfreegames/libpitaya-cluster/compare/0.2.2...0.3.10
[0.3.0]: https://github.com/topfreegames/libpitaya-cluster/compare/0.3.0-beta.2...0.3.0
[0.2.2]: https://github.com/topfreegames/libpitaya-cluster/releases/tag/0.2.2
