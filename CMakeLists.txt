project(StatsdClient)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CPP_STATSD_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/cpp-statsd-client)

include_directories(${CPP_STATSD_CLIENT_DIR})

install(DIRECTORY ${CPP_STATSD_CLIENT_DIR} DESTINATION include)

find_package(Threads)
add_executable(testStatsdClient ${CMAKE_CURRENT_SOURCE_DIR}/tests/testStatsdClient.cpp)
target_link_libraries(testStatsdClient ${CMAKE_THREAD_LIBS_INIT})

enable_testing()
add_test(testSuite testStatsdClient)

unset(CPP_STATSD_CLIENT_DIR)
