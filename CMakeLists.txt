project(StatsdClient)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cpp-statsd-client)

install(FILES UDPSender.hpp StatsdClient.hpp DESTINATION include)

find_package(Threads)
add_executable(testStatsdClient ${CMAKE_CURRENT_SOURCE_DIR}/tests/testStatsdClient.cpp)
target_link_libraries(testStatsdClient ${CMAKE_THREAD_LIBS_INIT})

enable_testing()
add_test(testSuite testStatsdClient)