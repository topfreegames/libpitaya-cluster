[WIP] pitaya-shared
=============

develop servers capable of discovering other pitaya servers and sending/receiving RPC in other programming languages.

### Requirements
- Dotnet and Mono (for running csharp example)
- Docker (for starting dependencies)

### Pitaya Dependencies
for running go-server, csharp-example and unity-example you must have an etcd running into localhost, port 2379 and a nats instance running on port 4222, theres a docker-compose file in the project with these dependencies, you can start them with ```make start-deps```

### About the components of the project
- **cpp-lib**: there's the source code for the c-shared library
- **csharp-lib**: this libs wraps the shared library methods and you must compile it into a dll and include it in csharp projects, you can use this command to build: ```make build-csharp-lib```
- **python-lib**: this a python lib that wraps the shared library methods, you can include it in python projects to create python pitaya servers
- **go-server**: thats an example server for using with the other components, you can run it with ```make run-go-server```
- **csharp-example**: this is an example that uses csharp-lib, for running it you must place (or link) out/libpitaya_cluster.dylib into its folder
- **unity-example**: this is an unity example that uses csharp-lib, for running it you must place (or link) out/libpitaya_cluster.dylib into Assets/Plugins folder

### How to build the c++ library
The C++ library can be built and used from C++ or could also be used from another programming language that can interoperate with C. The library uses [conan](https://conan.io) and git submodules in order to install the dependencies.

After having conan installed, building the library should be as easy as running the make targets inside `cpplib`:

```bash
make build-mac-unity
make build-mac
make build-linux
```

This targets will build for MacOS for usage in Unity, MacOS and Linux, in that order. If you are using MacOS and want to build for Linux without a VM, you can use docker for that. Simply run the following make targets:

```bash
make build-docker-image
make build-linux-docker
```

The binaries will be placed in the `_builds` folder, where each subdirectory will correspond to a different make target.

**Note**: We currently do not support windows. However, it should however not be hard to add support for it. Feel free to make a PR!
