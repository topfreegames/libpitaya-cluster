cmake_minimum_required(VERSION 3.7)

project(PitayaCluster)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

add_subdirectory(deps/grpc EXCLUDE_FROM_ALL)
add_subdirectory(deps/etcd-cpp-apiv3 EXCLUDE_FROM_ALL)

add_library(pitaya_cluster SHARED
    include/service_discovery.h
    include/pitaya_nats.h
    src/service_discovery.cpp
    src/nats_rpc_server.cpp
    src/string_utils.cpp
    src/string_utils.h
    include/sync_map.h)

add_library(pitaya::pitaya_cluster ALIAS pitaya_cluster)

set_target_properties(pitaya_cluster
  PROPERTIES
    CXX_STANDARD 14)

target_include_directories(pitaya_cluster
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
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
