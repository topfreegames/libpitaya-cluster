cmake_minimum_required(VERSION 3.26.3)

project(PitayaIntegrationTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Find CNATS from Conan
find_package(cnats REQUIRED)

# Find OpenSSL from Conan (optional, CNATS may need it)
find_package(OpenSSL QUIET)

# Create simple test executable
add_executable(simple_test
    src/simple_test.cpp
)

# Create responder service executable
add_executable(responder
    src/responder.cpp
)

# Create lame duck zombie test executable
add_executable(lame_duck_zombie_test
    src/lame_duck_zombie_test.cpp
)

# Include directories (using Conan targets)
target_include_directories(simple_test PRIVATE src)
target_include_directories(responder PRIVATE src)
target_include_directories(lame_duck_zombie_test PRIVATE src)

# Link libraries using Conan targets
target_link_libraries(simple_test PRIVATE
    cnats::nats_static
    Threads::Threads
)

target_link_libraries(responder PRIVATE
    cnats::nats_static
    Threads::Threads
)

target_link_libraries(lame_duck_zombie_test PRIVATE
    cnats::nats_static
    Threads::Threads
)

# Link OpenSSL if found
if(OpenSSL_FOUND)
    target_link_libraries(simple_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(responder PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(lame_duck_zombie_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Additional libraries for Apple
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(SECURITY Security)
    if(CORE_FOUNDATION AND SECURITY)
        target_link_libraries(simple_test PRIVATE ${CORE_FOUNDATION} ${SECURITY})
        target_link_libraries(responder PRIVATE ${CORE_FOUNDATION} ${SECURITY})
        target_link_libraries(lame_duck_zombie_test PRIVATE ${CORE_FOUNDATION} ${SECURITY})
    endif()
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

set_target_properties(lame_duck_zombie_test PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME lame_duck_zombie_test
)
