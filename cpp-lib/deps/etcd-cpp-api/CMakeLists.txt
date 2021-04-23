cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_THREAD_PREFER_PTHREAD)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)

set(etcd-cpp-api_VERSION_MAJOR 0)
set(etcd-cpp-api_VERSION_MINOR 1)

# Use native optimizations
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
endif()

set(SRC_ROOT "${PROJECT_SOURCE_DIR}/src")
set(HDR_ROOT "${PROJECT_SOURCE_DIR}/include/")
set(TST_ROOT "${PROJECT_SOURCE_DIR}/test")

#include_directories(SYSTEM ${Boost_INCLUDE_DIR})
include_directories(${HDR_ROOT})

add_subdirectory(${SRC_ROOT})

# Tests
if(BUILD_TESTING)
    set(TEST_PROJECT test_${PROJECT_NAME})
    add_test(NAME ${TEST_PROJECT} COMMAND ${TEST_PROJECT})
    add_subdirectory(${TST_ROOT})
    enable_testing()
endif()
