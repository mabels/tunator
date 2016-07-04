cmake_minimum_required (VERSION 2.8)
project (Tunator)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version >=4.8 required.")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
        message(FATAL_ERROR "Clang version >=3.4 required.")
    endif()
else()
    message(WARNING "Your compiler (${CMAKE_CXX_COMPILER_ID}) has not been tested on this project. Only Clang and GCC has been tested. Please report any problems at the project page on GitHub.")
endif()

#Only tested with versions 1.55 and 1.56
find_package(Boost 1.54.0 COMPONENTS system regex coroutine context thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if(APPLE)
  set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

set(SIMPLEWEB_INCLUDE_DIR "/Users/menabe/Software/Simple-WebSocket-Server")
#TODO: add requirement for version 1.0.1g (can it be done in one line?)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${SIMPLEWEB_INCLUDE_DIR})

find_package(Threads REQUIRED)

include_directories(.)

add_executable(tunator tunator.cpp)
target_link_libraries(tunator ${Boost_LIBRARIES})
target_link_libraries(tunator ${OPENSSL_LIBRARIES})
target_link_libraries(tunator ${CMAKE_THREAD_LIBS_INIT})

if(MSYS)
    target_link_libraries(tunator ws2_32 wsock32)
endif()

#enable_testing()
#add_subdirectory(test)