cmake_minimum_required(VERSION 3.7)

project(PitayaCluster)

# Prefer static libraries over dynamic ones.
if(UNIX AND NOT APPLE)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
  # FIXME: this is an ugly solution, avoid setting global variables
  # by adding properties to the targets.
  set(CMAKE_CXX_FLAGS "-fPIC")
  set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cpprestsdk_install/linux")
elseif(APPLE)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dylib" ".so" ".dll")
  set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cpprestsdk_install/mac")
else()
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dylib" ".so" ".dll")
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

find_package(Boost REQUIRED)

# Build dependencies as static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "")
if(UNIX AND NOT APPLE)
    set(gRPC_ZLIB_PROVIDER "package")
endif()
add_subdirectory(deps/grpc EXCLUDE_FROM_ALL)

add_subdirectory(deps/etcd-cpp-apiv3 EXCLUDE_FROM_ALL)

set(NATS_BUILD_STREAMING OFF CACHE BOOL "")
add_subdirectory(deps/cnats-1.8.0 EXCLUDE_FROM_ALL)

option(BUILD_MACOSX_BUNDLE "Should build a bundle for Macosx" OFF)

if(BUILD_MACOSX_BUNDLE)
    message(STATUS "Setting library to MODULE")
    set(LIB_TYPE MODULE)
elseif(BUILD_TESTING)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

file(GLOB protobuf_files
    "include/pitaya/protos/*")

add_library(pitaya_cluster ${LIB_TYPE}
    include/pitaya.h
    include/pitaya/constants.h
    include/pitaya/cluster.h
    include/pitaya/c_wrapper.h

    include/pitaya/service_discovery.h
    include/pitaya/etcdv3_service_discovery.h
    include/pitaya/etcdv3_service_discovery/config.h
    include/pitaya/etcd_client.h
    include/pitaya/etcd_lease_keep_alive.h

    include/pitaya/rpc_client.h
    include/pitaya/rpc_server.h
    include/pitaya/nats/config.h
    include/pitaya/nats/rpc_client.h
    include/pitaya/nats/rpc_server.h
    include/pitaya/grpc/rpc_client.h
    include/pitaya/grpc/rpc_server.h
    include/pitaya/grpc/config.h

    include/pitaya/utils.h
    include/pitaya/utils/semaphore.h
    include/pitaya/utils/ticker.h
    include/pitaya/utils/sync_map.h
    include/pitaya/utils/sync_deque.h
    include/pitaya/utils/sync_vector.h

    src/pitaya.cpp
    src/pitaya/etcdv3_service_discovery.cpp
    src/pitaya/etcdv3_service_discovery/worker.cpp
    src/pitaya/etcdv3_service_discovery/worker.h
    src/pitaya/nats/rpc_server.cpp
    src/pitaya/nats/rpc_client.cpp
    src/pitaya/grpc/rpc_client.cpp
    src/pitaya/grpc/rpc_server.cpp
    src/pitaya/utils/string_utils.cpp
    src/pitaya/cluster.cpp
    src/pitaya/utils.cpp
    src/pitaya/utils/grpc.h
    src/pitaya/utils/grpc.cpp
    src/pitaya/utils/string_utils.h
    src/pitaya/utils/ticker.cpp
    src/pitaya/c_wrapper.cpp
    src/pitaya/etcd_lease_keep_alive.cpp
    src/pitaya/etcd_client.cpp
    # Include protobuf-c
    deps/protobuf-c/protobuf-c/protobuf-c.c
    deps/protobuf-c/protobuf-c/protobuf-c.h
    ${protobuf_files})

add_library(pitaya::pitaya_cluster ALIAS pitaya_cluster)

if(BUILD_MACOSX_BUNDLE)
    message(STATUS "Setting properties for MacOSX bundle")
    set_target_properties(pitaya_cluster
      PROPERTIES
        SUFFIX ".bundle")
endif()

set_target_properties(pitaya_cluster
  PROPERTIES
    CXX_STANDARD 11
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(pitaya_cluster
  PUBLIC
    $<BUILD_INTERFACE:
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/deps/spdlog/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src)

target_link_libraries(pitaya_cluster
  PUBLIC
    nats_static
    etcd-cpp-api
    Boost::boost
  PRIVATE
    libprotobuf)

include(CTest)
if(BUILD_TESTING AND NOT BUILD_MACOSX_BUNDLE)
    # ===============================
    # Add example
    # ===============================
    add_executable(main src/pitaya_cluster.cpp)

    target_link_libraries(main PRIVATE pitaya_cluster)

    set_target_properties(main PROPERTIES CXX_STANDARD 11)

    #==============================
    # Enable tests
    #==============================
    find_package(gtest REQUIRED)
    find_package(Boost REQUIRED COMPONENTS system thread)

    add_executable(pitaya_cluster_tests
        test/test_common.h
        test/mock_service_discovery.h
        test/mock_rpc_client.h
        test/mock_rpc_server.h
        test/mock_etcd_client.h
        test/cluster_test.cpp
        test/ticker_test.cpp
        test/etcdv3_service_discovery_test.cpp
        test/utils_test.cpp
        test/grpc_client_test.cpp
        test/grpc_server_test.cpp
        test/main_test.cpp)

    message(WARNING "Adding profiling data into the library and the tests executable, do not ship this library to production (compile with -DBUILD_TESTING=OFF)")

    target_compile_options(pitaya_cluster
      PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0)

    target_link_libraries(pitaya_cluster
      PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    target_include_directories(pitaya_cluster_tests PUBLIC src)

    set_target_properties(pitaya_cluster_tests PROPERTIES
        CXX_STANDARD 14
        OUTPUT_NAME tests)

    target_link_libraries(pitaya_cluster_tests
      PRIVATE
        pitaya_cluster
        Boost::boost
        Boost::system
        Boost::thread
        CONAN_PKG::gtest)

    add_test(tests tests)
endif()
