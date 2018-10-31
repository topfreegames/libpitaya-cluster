cmake_minimum_required(VERSION 3.7)

project(PitayaCluster)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

add_subdirectory(deps/grpc EXCLUDE_FROM_ALL)
add_subdirectory(deps/etcd-cpp-apiv3 EXCLUDE_FROM_ALL)

file(GLOB protobuf_files
  "include/protos/*"
  )

add_library(pitaya_cluster SHARED
    include/pitaya.h
    include/pitaya/service_discovery.h
    include/pitaya/service_discovery/worker.h
    include/pitaya/nats/rpc_client.h
    include/pitaya/nats/rpc_server.h
    include/pitaya/nats/config.h
    include/pitaya/cluster.h
    include/pitaya/utils/semaphore.h
    include/pitaya/utils/ticker.h
    include/pitaya/utils/sync_map.h
    include/pitaya/utils/sync_deque.h
    include/pitaya/service_discovery/lease_keep_alive.h
    src/pitaya/service_discovery.cpp
    src/pitaya/service_discovery/worker.cpp
    src/pitaya/nats/rpc_server.cpp
    src/pitaya/nats/rpc_client.cpp
    src/pitaya/utils/string_utils.cpp
    src/pitaya/cluster.cpp
    src/pitaya/utils.cpp
    src/pitaya/utils/string_utils.h
    src/pitaya/utils/ticker.cpp
    src/pitaya/service_discovery/lease_keep_alive.cpp
    # Include protobuf-c
    deps/protobuf-c/protobuf-c/protobuf-c.c
    deps/protobuf-c/protobuf-c/protobuf-c.h
    ${protobuf_files}
    )

add_library(pitaya::pitaya_cluster ALIAS pitaya_cluster)

set_target_properties(pitaya_cluster
  PROPERTIES
    CXX_STANDARD 17)

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
    CONAN_PKG::cnats
    etcd-cpp-api)

# ===============================
# Add example
# ===============================

add_executable(main
    src/pitaya_cluster.cpp)

target_link_libraries(main PRIVATE pitaya_cluster)

set_target_properties(main
  PROPERTIES
    CXX_STANDARD 14)
