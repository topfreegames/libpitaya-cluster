cmake_minimum_required(VERSION 3.7)
project(jaeger_cpp_client)
include(ExternalProject)

ExternalProject_Add(jaeger_cpp_client
    GIT_REPOSITORY "https://github.com/jaegertracing/jaeger-client-cpp"
    GIT_TAG master
    SOURCE_DIR "${CMAKE_BINARY_DIR}/jaeger-client-cpp-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/jaeger-client-cpp-build"
    INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
    TEST_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/install
)

