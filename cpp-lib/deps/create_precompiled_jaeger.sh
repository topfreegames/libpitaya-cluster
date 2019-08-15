#!/bin/bash

platform=$1

function print_usage {
    echo "Usage: ./create_precompiled_jaeger.sh macosx|linux|linux-docker"
}

function build {
    cd jaeger-cpp-client
    rm -rf build && conan install . -if build && cd build
    cmake .. -GNinja \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DJAEGERTRACING_BUILD_CROSSDOCK=OFF \
        -DJAEGERTRACING_COVERAGE=OFF \
        -DJAEGERTRACING_WITH_YAML_CPP=OFF \
        -DCMAKE_INSTALL_PREFIX=../../jaegertracing_install_$platform
    cmake --build . --target install
}

function build_linux_docker {
    docker run -ti -v $(pwd):/app pitaya-cluster /bin/bash -c "cd /app && conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan && ./create_precompiled_jaeger.sh linux"
}

case $platform in
    macosx|linux)
        build
        ;;
    linux-docker)
        build_linux_docker
        ;;
    *)
        print_usage
        ;;
esac

