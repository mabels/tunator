cmake_minimum_required(VERSION 2.8.7)
project (TunatorTest)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TunatorTest_SOURCE_DIR}/build)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wunused-parameter -g")


find_package(Boost 1.54.0 COMPONENTS system regex coroutine context thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(Threads REQUIRED)

add_executable(packet_statistic_test packet_statistic_test.cpp)
add_test(packet_statistic_test packet_statistic_test)

add_executable(packet_buffer_test packet_buffer_test.cpp)
add_test(packet_buffer_test packet_buffer_test)

add_executable(packet_queue_test packet_queue_test.cpp)
add_test(packet_queue_test packet_queue_test)

add_executable(if_addrs_test if_addrs_test.cpp)
add_test(if_addrs_test if_addrs_test)
target_link_libraries(if_addrs_test ${jsoncpp_LIBRARIES})

add_executable(tun_device_test tun_device_test.cpp ../tun_device.cpp)
target_link_libraries(tun_device_test ${Boost_LIBRARIES})
add_test(tun_device_test tun_device_test)

add_executable(tuna_server_test tuna_server_test.cpp ../tunator.cpp ../tun_device.cpp)
target_link_libraries(tuna_server_test ${jsoncpp_LIBRARIES})
target_link_libraries(tuna_server_test ${Boost_LIBRARIES})
target_link_libraries(tuna_server_test ${OPENSSL_LIBRARIES})
add_test(tuna_server_test tuna_server_test)
