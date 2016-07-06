cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

add_executable(packet_statistic_test packet_statistic_test.cpp)
add_test(packet_statistic_test packet_statistic_test)
