cmake_minimum_required(VERSION 3.7)

project(PitayaCluster)

set(CMAKE_CXX_FLAGS "-fPIC")

# Prefer static libraries over dynamic ones.
if(UNIX AND NOT APPLE)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
else()
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dylib" ".so" ".dll")
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

find_package(Boost REQUIRED)

add_subdirectory(deps/grpc EXCLUDE_FROM_ALL)
add_subdirectory(deps/etcd-cpp-apiv3 EXCLUDE_FROM_ALL)
add_subdirectory(deps/cnats-1.8.0 EXCLUDE_FROM_ALL)

file(GLOB protobuf_files
  "include/protos/*")

option(BUILD_MACOSX_BUNDLE "Should build a bundle for Macosx" OFF)

if(BUILD_MACOSX_BUNDLE)
    message(STATUS "Setting library to MODULE")
    set(LIB_TYPE MODULE)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(pitaya_cluster ${LIB_TYPE}
    include/pitaya.h
    include/pitaya/constants.h
    include/pitaya/cluster.h

    include/pitaya/service_discovery.h
    include/pitaya/etcdv3_service_discovery.h
    include/pitaya/etcdv3_service_discovery/worker.h
    include/pitaya/etcdv3_service_discovery/config.h
    include/pitaya/etcdv3_service_discovery/lease_keep_alive.h

    include/pitaya/rpc_client.h
    include/pitaya/rpc_server.h
    include/pitaya/nats/config.h
    include/pitaya/nats/rpc_client.h
    include/pitaya/nats/rpc_server.h

    include/pitaya/utils/semaphore.h
    include/pitaya/utils/ticker.h
    include/pitaya/utils/sync_map.h
    include/pitaya/utils/sync_deque.h

    src/pitaya/etcdv3_service_discovery.cpp
    src/pitaya/etcdv3_service_discovery/worker.cpp
    src/pitaya/etcdv3_service_discovery/lease_keep_alive.cpp
    src/pitaya/nats/rpc_server.cpp
    src/pitaya/nats/rpc_client.cpp
    src/pitaya/utils/string_utils.cpp
    src/pitaya/cluster.cpp
    src/pitaya/utils.cpp
    src/pitaya/c_wrapper.cpp
    src/pitaya/utils/string_utils.h
    src/pitaya/utils/ticker.cpp
    # Include protobuf-c
    deps/protobuf-c/protobuf-c/protobuf-c.c
    deps/protobuf-c/protobuf-c/protobuf-c.h
    ${protobuf_files}
    )

add_library(pitaya::pitaya_cluster ALIAS pitaya_cluster)

if(BUILD_MACOSX_BUNDLE)
    message(STATUS "Setting properties for MacOSX bundle")
    set_target_properties(pitaya_cluster
        PROPERTIES
            SUFFIX ".bundle"
            #BUNDLE TRUE
            )
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
  PRIVATE
    libprotobuf
    Boost::boost)

# ===============================
# Add example
# ===============================

if(NOT BUILD_MACOSX_BUNDLE)
    add_executable(main
        src/pitaya_cluster.cpp)

    target_link_libraries(main PRIVATE pitaya_cluster)

    set_target_properties(main
      PROPERTIES
        CXX_STANDARD 11)
endif()

#==============================
# Enable tests
#==============================
include(CTest)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(pitaya_cluster_tests
        test/mock_service_discovery.h
        test/mock_rpc_client.h
        test/mock_rpc_server.h
        test/cluster_test.cpp
        test/main_test.cpp)

    set_target_properties(pitaya_cluster_tests PROPERTIES
        OUTPUT_NAME tests)

    target_link_libraries(pitaya_cluster_tests
      PRIVATE GTest::GTest GTest::Main pitaya_cluster)

    add_test(tests tests)
endif()
