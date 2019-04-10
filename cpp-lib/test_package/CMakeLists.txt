cmake_minimum_required(VERSION 3.8)

project(pitaya_cpp_test)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

add_executable(example example.cpp)

set_target_properties(example PROPERTIES CXX_STANDARD 11)

target_link_libraries(example PUBLIC CONAN_PKG::pitaya_cpp)
