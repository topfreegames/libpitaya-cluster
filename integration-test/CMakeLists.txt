cmake_minimum_required(VERSION 3.26.3)

project(PitayaSimpleIntegrationTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Use exact path to CNATS library
set(CNATS_LIB "/Users/hspedro/.conan2/p/b/cnats390e964bbd2f6/p/lib/libnats_static.a")
set(CNATS_INCLUDE_DIR "/Users/hspedro/.conan2/p/b/cnats390e964bbd2f6/p/include")

message(STATUS "Using CNATS library: ${CNATS_LIB}")
message(STATUS "Using CNATS headers: ${CNATS_INCLUDE_DIR}")

# Create simple test executable
add_executable(simple_test
    src/simple_test.cpp
)

# Create responder service executable
add_executable(responder
    src/responder.cpp
)

# Include directories
target_include_directories(simple_test PRIVATE
    ${CNATS_INCLUDE_DIR}
    src
)

target_include_directories(responder PRIVATE
    ${CNATS_INCLUDE_DIR}
    src
)

# Link libraries
target_link_libraries(simple_test PRIVATE
    ${CNATS_LIB}
    Threads::Threads
)

target_link_libraries(responder PRIVATE
    ${CNATS_LIB}
    Threads::Threads
)

# Additional libraries that CNATS might need
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(SECURITY Security)
    target_link_libraries(simple_test PRIVATE ${CORE_FOUNDATION} ${SECURITY})
    target_link_libraries(responder PRIVATE ${CORE_FOUNDATION} ${SECURITY})
endif()

# Find OpenSSL (CNATS may need it)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_link_libraries(simple_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(responder PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Set properties
set_target_properties(simple_test PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME simple_test
)

set_target_properties(responder PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME responder
)