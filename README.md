[WIP] pitaya-shared
=============

Develop servers capable of discovering other pitaya servers and sending/receiving RPC in other programming languages.

### Requirements
- Go
- Dotnet (for running csharp example)

### Running the examples
* build the shared lib ```make```
* copy libpitaya_cluster.dylib from "out" folder to the example folder
* run the example, e.g. ```make run-csharp-example```
