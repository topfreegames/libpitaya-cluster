cmake_minimum_required(VERSION 3.26.3)

include(FetchContent)

project(PitayaCpp)

set(CMAKE_MACOSX_RPATH 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer static libraries over dynamic ones.
if(UNIX AND NOT APPLE)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
else()
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dylib" ".so" ".dll")
endif()

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(cpprestsdk REQUIRED)
include_directories(${cpprestsdk_INCLUDE_DIRS})

FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG v1.13.0
)
# Download spdlog
FetchContent_MakeAvailable(spdlog)
set_target_properties(spdlog PROPERTIES COMPILE_FLAGS -fPIC)

find_package(gRPC REQUIRED)
include_directories(${gRPC_INCLUDE_DIRS})

find_package(Protobuf REQUIRED)
include_directories(${protobuf_INCLUDE_DIRS})

find_package(protobuf-c REQUIRED)
include_directories(${protobuf-c_INCLUDE_DIRS})

find_package(absl REQUIRED)
include_directories(${absl_INCLUDE_DIRS})

find_package(etcd-cpp-apiv3 REQUIRED)
include_directories(${etcd-cpp-apiv3_INCLUDE_DIRS})

# Build dependencies as static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "")
if(UNIX AND NOT APPLE)
    set(gRPC_ZLIB_PROVIDER "package")
endif()

set(NATS_BUILD_STREAMING OFF CACHE BOOL "")
set(NATS_BUILD_TLS_USE_OPENSSL_1_1_API ON CACHE BOOL "" FORCE)
add_subdirectory(deps/nats.c-3.8.0 EXCLUDE_FROM_ALL)

set_target_properties(nats_static PROPERTIES COMPILE_FLAGS -fPIC)
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

add_library(pitaya_cpp ${LIB_TYPE}
    include/pitaya.h
    include/pitaya/constants.h
    include/pitaya/cluster.h
    include/pitaya/c_wrapper.h
    include/pitaya/binding_storage.h

    include/pitaya/service_discovery.h
    include/pitaya/etcd_config.h
    include/pitaya/nats_client.h

    include/pitaya/rpc_client.h
    include/pitaya/rpc_server.h
    include/pitaya/nats_config.h
    include/pitaya/grpc_config.h

    include/pitaya/utils.h
    include/pitaya/utils/semaphore.h
    include/pitaya/utils/ticker.h
    include/pitaya/utils/sync_map.h
    include/pitaya/utils/sync_deque.h
    include/pitaya/utils/sync_vector.h

    src/pitaya.cpp
    src/pitaya/etcd_client.h
    src/pitaya/etcd_client_v3.h
    src/pitaya/etcd_client_v3.cpp
    src/pitaya/etcdv3_service_discovery.h
    src/pitaya/etcdv3_service_discovery.cpp
    src/pitaya/etcdv3_service_discovery/worker.cpp
    src/pitaya/etcdv3_service_discovery/worker.h
    src/pitaya/etcd_binding_storage.h
    src/pitaya/etcd_binding_storage.cpp
    src/pitaya/nats/rpc_server.h
    src/pitaya/nats/rpc_server.cpp
    src/pitaya/nats/rpc_client.h
    src/pitaya/nats/rpc_client.cpp
    src/pitaya/grpc/rpc_client.h
    src/pitaya/grpc/rpc_client.cpp
    src/pitaya/grpc/rpc_server.h
    src/pitaya/grpc/rpc_server.cpp
    src/pitaya/utils/string_utils.cpp
    src/pitaya/cluster.cpp
    src/pitaya/utils.cpp
    src/pitaya/utils/grpc.h
    src/pitaya/utils/grpc.cpp
    src/pitaya/utils/string_utils.h
    src/pitaya/utils/ticker.cpp
    src/pitaya/c_wrapper.cpp
    src/pitaya/nats_client.cpp
    ${protobuf_files})

add_library(TFG::pitaya_cpp ALIAS pitaya_cpp)

if(BUILD_MACOSX_BUNDLE)
    message(STATUS "Setting properties for MacOSX bundle")
    set_target_properties(pitaya_cpp
      PROPERTIES
        SUFFIX ".bundle")
endif()

set_target_properties(pitaya_cpp
  PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(pitaya_cpp
  PUBLIC
    $<BUILD_INTERFACE:
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src)

target_link_libraries(pitaya_cpp
  PUBLIC
    nats_static
    etcd-cpp-apiv3::etcd-cpp-apiv3
    Boost::boost
    spdlog::spdlog
    protobuf-c::protobuf-c
  PRIVATE
    gRPC::grpc++_unsecure
    protobuf::libprotobuf)

include(CTest)
if(BUILD_TESTING AND NOT BUILD_MACOSX_BUNDLE)
    # ===============================
    # Add example
    # ===============================
    add_executable(main src/pitaya_cluster.cpp)

    target_link_libraries(main PRIVATE pitaya_cpp)

    set_target_properties(main PROPERTIES CXX_STANDARD 17)

    #==============================
    # Enable tests
    #==============================
    find_package(GTest REQUIRED)
    include_directories(${GTest_INCLUDE_DIRS})
    find_package(Boost REQUIRED COMPONENTS system thread)

    add_executable(pitaya_cpp_tests
        test/test_common.h
        test/mock_service_discovery.h
        test/mock_rpc_client.h
        test/mock_rpc_server.h
        test/mock_etcd_client.h
        test/mock_binding_storage.h
        test/mock_nats_client.h
        test/cluster_test.cpp
        test/ticker_test.cpp
        test/nats_rpc_client_test.cpp
        test/nats_rpc_server_test.cpp
        test/etcdv3_service_discovery_test.cpp
        test/utils_test.cpp
        test/grpc_client_test.cpp
        test/grpc_server_test.cpp
        test/c_wrapper_test.cpp
        test/main_test.cpp)

    message(INFO "Adding profiling data into the library and the tests executable, do not ship this library to production (compile with -DBUILD_TESTING=OFF)")

    target_compile_options(pitaya_cpp
      PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0)

    target_link_libraries(pitaya_cpp
      PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    target_include_directories(pitaya_cpp_tests PUBLIC src)

    set_target_properties(pitaya_cpp_tests PROPERTIES
        CXX_STANDARD 17
        OUTPUT_NAME tests)

    target_link_libraries(pitaya_cpp_tests
      PRIVATE
        pitaya_cpp
        Boost::boost
        Boost::system
        Boost::thread
        GTest::gmock
        GTest::gtest)

    add_test(tests tests)
endif()

#------------------------------------------------------
# Installation configuration for pitaya cpp
#------------------------------------------------------
install(
  TARGETS pitaya_cpp
  # EXPORT ${PROJECT_NAME}Targets
  INCLUDES DESTINATION "include"
  LIBRARY DESTINATION "lib/${PROJECT_NAME}"
  ARCHIVE DESTINATION "lib/${PROJECT_NAME}"
  RUNTIME DESTINATION "bin")

install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  DESTINATION "include")

