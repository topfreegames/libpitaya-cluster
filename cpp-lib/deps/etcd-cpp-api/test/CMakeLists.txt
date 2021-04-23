cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)

project(${TEST_PROJECT})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED)

set(TEST_SOURCES ${TST_ROOT}/EtcdTest.cpp
		${TST_ROOT}/EtcdSyncTest.cpp
		${TST_ROOT}/WatcherTest.cpp)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})
#add_executable(${PROJECT_NAME} ${TST_ROOT}/wrapper_test.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE etcd-cpp-api Boost::boost)

