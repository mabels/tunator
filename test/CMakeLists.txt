cmake_minimum_required(VERSION 2.8.7)
project (TunatorTest)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TunatorTest_SOURCE_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wextra -DTEST")
if (${CMAKE_COMPILER_IS_GNUCXX})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -g")

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    #  set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()


#set(CMAKE_SHARED_LINKER_FLAGS="-lprofiler -ltcmalloc")


enable_testing()

find_package(Boost 1.54.0 COMPONENTS system regex coroutine context iostreams thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(Threads REQUIRED)

add_executable(packet_statistic_test packet_statistic_test.cpp)
add_test(packet_statistic_test packet_statistic_test)

add_executable(packet_buffer_test packet_buffer_test.cpp)
add_test(packet_buffer_test packet_buffer_test)

add_executable(packet_queue_test packet_queue_test.cpp)
target_link_libraries(packet_queue_test ${CMAKE_THREAD_LIBS_INIT})
add_test(packet_queue_test packet_queue_test)


add_executable(if_addrs_test_linux if_addrs_test.cpp ../if_addrs_linux.cpp)
add_test(if_addrs_test_linux if_addrs_test_linux)
target_link_libraries(if_addrs_test_linux ${ipaddress_LIBRARIES})
target_link_libraries(if_addrs_test_linux ${Boost_LIBRARIES})
target_link_libraries(if_addrs_test_linux ${jsoncpp_LIBRARIES})

add_executable(if_addrs_test_macos if_addrs_test.cpp ../if_addrs_macos.cpp)
add_test(if_addrs_test_macos if_addrs_test_macos)
target_link_libraries(if_addrs_test_macos ${ipaddress_LIBRARIES})
target_link_libraries(if_addrs_test_macos ${Boost_LIBRARIES})
target_link_libraries(if_addrs_test_macos ${jsoncpp_LIBRARIES})


add_executable(tun_device_test tun_device_test.cpp ../tun_device.cpp ../if_addrs_linux.cpp)
target_link_libraries(tun_device_test ${Boost_LIBRARIES})
target_link_libraries(tun_device_test ${CMAKE_THREAD_LIBS_INIT})
add_test(tun_device_test tun_device_test)

add_executable(tuna_server_test tuna_server_test.cpp ../tunator.cpp ../tun_device.cpp ../if_addrs_linux.cpp)
target_link_libraries(tuna_server_test ${ipaddress_LIBRARIES})
target_link_libraries(tuna_server_test ${jsoncpp_LIBRARIES})
target_link_libraries(tuna_server_test ${Boost_LIBRARIES})
target_link_libraries(tuna_server_test ${OPENSSL_LIBRARIES})
target_link_libraries(tuna_server_test ${CMAKE_THREAD_LIBS_INIT})
add_test(tuna_server_test tuna_server_test)

add_executable(linux_ip_route_test linux_ip_route_test.cpp)
target_link_libraries(linux_ip_route_test ${ipaddress_LIBRARIES})
target_link_libraries(linux_ip_route_test ${Boost_LIBRARIES})
target_link_libraries(linux_ip_route_test ${CMAKE_THREAD_LIBS_INIT})
add_test(linux_ip_route_test linux_ip_route_test)


add_executable(system_cmd_test system_cmd_test.cpp)
target_link_libraries(system_cmd_test ${Boost_LIBRARIES})
target_link_libraries(system_cmd_test ${CMAKE_THREAD_LIBS_INIT})
add_test(system_cmd_test system_cmd_test)
