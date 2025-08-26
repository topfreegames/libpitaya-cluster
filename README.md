# [WIP] pitaya-server

## Overview
The original [Pitaya](https://github.com/topfreegames/pitaya) project supports building pitaya servers in Go. This project aims to provide the same functionality, however aimed towards other programming languages. Currently, however, the supported languages are C++ and C#.

**Note**: This library is still in early stage, meaning the there might be several bugs. Also, the API is not stable.

<!-- ## Requirements -->
<!-- - Dotnet and Mono (for running csharp example) -->
<!-- - Docker (for starting testing dependencies) -->
<!-- - Cmake >= 3.23 for building -->
<!-- - Conan >= 2.x for building -->

<!-- ## Pitaya Dependencies -->
<!-- for running go-server, csharp-example and unity-example you must have an etcd running into localhost, port 2379 and a nats instance running on port 4222, theres a docker-compose file in the project with these dependencies, you can start them with ```make start-deps``` -->

## About the components of the project
- **cpp-lib**: the C++ core library.
- **pitaya-sharp**: this is a solution with multiple C# projects. The main one is `NPitaya`, a library that wraps the native C++ library and provides a conveninent interface for writing pitaya servers in C#.
- **python-lib**: this a python lib that wraps the shared library methods, you can include it in python projects to create python pitaya servers
- **go-server**: thats an example server for using with the other components, you can run it with ```make run-go-server```
- **unity-example**: this is an unity example that uses NPitaya. For running it you must place (or link) out/libpitaya_cluster.dylib into Assets/Plugins folder

## Documentation

### C++ Library Documentation
- **[NATS Lame Duck Mode Handling](cpp-lib/docs/LAME_DUCK_MODE.md)**: Comprehensive guide on how the NATS client handles graceful server shutdowns, including automatic message buffering, thread-safe operations, and reconnection strategies.

## Installation
| Language | Project Location             |
| -------- |------------------------------|
| C++      | [cpp-lib](cpp-lib)           |
| C#       | [pitaya-sharp](pitaya-sharp) |

<!-- ### C++ -->
<!-- The C++ library can be built and used from C++ or could also be used from another programming language that can interoperate with C. The library uses [conan](https://conan.io) and git submodules in order to install the dependencies. -->

<!-- After having conan installed, building the library should be as easy as running the make targets inside `cpplib`: -->

<!-- ```bash -->
<!-- make build-mac-unity -->
<!-- make build-mac -->
<!-- make build-linux -->
<!-- ``` -->

<!-- This targets will build for MacOS for usage in Unity, MacOS and Linux, in that order. If you are using MacOS and want to build for Linux without a VM, you can use docker for that. Simply run the following make targets: -->

<!-- ```bash -->
<!-- make build-docker-image -->
<!-- make build-linux-docker -->
<!-- ``` -->

<!-- The binaries will be placed in the `_builds` folder, where each subdirectory will correspond to a different make target. -->

<!-- **Note**: We currently do not support windows. However, it should however not be hard to add support for it. Feel free to make a PR! -->

## Releasing a new version in OpenUPM
- Wait for the Github Actions Pipeline to run, to generate the platform specific "libpitaya_cpp" binaries.
- Put the linux, windows, and macos (unity/fat) in the folder pitaya-sharp/NPitaya/Runtime/Plugins
- Change the version in the file pitaya-sharp/NPitaya/package.json
- Create a new tag in the format "vX.Y.Z" and push it to the repository

New lib version will be available in [OpenUPM](https://openupm.com/packages/com.wildlifestudios.npitaya/) in a few minutes. Read documentation [here](https://openupm.com/docs/#how-it-works) for more information.