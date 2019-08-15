#!/bin/bash

platform=$1

function print_usage {
    echo "Usage: ./create_precompiled_opentracing.sh macosx|linux|linux-docker"
}

function build {
    cd opentracing-cpp-1.5.1
    rm -rf build && mkdir build && cd build
    cmake .. -GNinja \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=../../opentracing_install_$platform
    cmake --build . --target install
}

function build_linux_docker {
    docker run -ti -v $(pwd):/app pitaya-cluster /bin/bash -c "cd /app && ./create_precompiled_opentracing.sh linux"
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

