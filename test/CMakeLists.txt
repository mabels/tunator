cmake_minimum_required(VERSION 2.8.7)
project (TunatorTest)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TunatorTest_SOURCE_DIR}/build)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -g")

find_package(Boost 1.54.0 COMPONENTS system regex coroutine context thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(packet_statistic_test packet_statistic_test.cpp)
add_test(packet_statistic_test packet_statistic_test)

add_executable(packet_buffer_test packet_buffer_test.cpp)
add_test(packet_buffer_test packet_buffer_test)

add_executable(packet_queue_test packet_queue_test.cpp)
add_test(packet_queue_test packet_queue_test)

add_executable(if_addrs_test if_addrs_test.cpp)
add_test(if_addrs_test if_addrs_test)
