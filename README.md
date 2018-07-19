[WIP] pitaya-shared
=============

develop servers capable of discovering other pitaya servers and sending/receiving RPC in other programming languages.

### Requirements
- Go (>= 1.11), there's a hack used in CGO before go 1.11 that will make processes linking to the lib crash if compiled with it
- Dotnet and Mono (for running csharp example)
- Docker (for starting dependencies)

### Pitaya Dependencies
for running go-server, csharp-example and unity-example you must have an etcd running into localhost, port 2379 and a nats instance running on port 4222, theres a docker-compose file in the project with these dependencies, you can start them with ```make start-deps```

### About the components of the project
- **root**: in the root, theres the source code for the c-shared library that we will build using go, you can use ```make build``` for building it, it will be placed into out folder
- **csharp-lib**: this libs wraps the shared library methods and you must compile it into a dll and include it in csharp projects, you can use this command to build: ```make build-csharp-lib```
- **python-lib**: this a python lib that wraps the shared library methods, you can include it in python projects to create python pitaya servers
- **go-server**: thats an example server for using with the other components, you can run it with ```make run-go-server```
- **csharp-example**: this is an example that uses csharp-lib, for running it you must place (or link) out/libpitaya_cluster.dylib into its folder
- **unity-example**: this is an unity example that uses csharp-lib, for running it you must place (or link) out/libpitaya_cluster.dylib into Assets/Plugins folder
