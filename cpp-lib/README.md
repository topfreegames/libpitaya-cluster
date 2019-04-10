# PitayaCpp

A library that allows you to to create Pitaya servers in C++.

## Installation
The library is currently distributed with [Conan](https://conan.io). To install it, you need to add the remote and add the dependency to your `conanfile.txt`:

```
pitaya_cpp/0.3.0@tfg/alpha.1
```

In the [examples](examples) folder you can find how to create a simple server using the `pitaya_cpp` library.

## API Documentation
*TODO*

## Building
If you want to build the library by yourself, you can do that with CMake (3.7 is the minimum version). There are convenient building targets in the `Makefile`, such as `make build-mac-release`, `make build-mac-unity` and `make build-linux-release`. If you want to provide the CMake variables yourself, these are the ones which have an impact on the build apart from the standard ones (e.g., CMAKE_BUILD_TYPE).

- `-DBUILD_TESTING`: This variable will build the tests for the library, however it will only build the library statically and will include code coverage support in the binary. This is therefore useful for developing and running tests, but should *not* be used as a production build. You can run the tests and open a code coverage window at the end of them with the script `run-tests-with-coverage.sh`. The scripts expects the executable location as a first argument.
- `-DBUILD_MACOSX_BUNDLE`: This is `OFF` by default. By enabling it, the library will be built with the `.bundle` extension. This is useful for running the library with Unity in MacOS.

A sample build could then be something like this:

```bash
conan install . -if build # install dependencies on the build folder
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON
cmake --build build
```

## Running tests
In order to run the tests you have to generate a build with support for them (see section above: `-DBUILD_TESTING=ON`). After that you can run the `tests` executable in the build folder. You can also see the code coverage with the `run-tests-with-coverage.sh <executable-path>` script.

## Limitations
- Currently there is no support for Windows. However building the library on Windows should not be too difficult. We will gladly accept PR's for this.
- The RPC server does not implement a convenient way of implementing handlers for C++ like it does for C#. What it means is that the main way of handling RPC's is by manually fetching RPCs with `Cluster::Instance().WaitForRpc`, and by finishing them with `pitaya::Rpc::Finish`. If you want multiple threads handling multiple RPCs this has to be done manually for now. We plan on adding better support for this in order to make building C++ servers easier.