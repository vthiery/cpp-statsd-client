cmake_minimum_required(VERSION 3.2)

project(StatsdClient)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

install(FILES src/UDPSender.hpp src/StatsdClient.hpp DESTINATION include)

find_package(Threads)
add_executable(testStatsdClient ${CMAKE_CURRENT_SOURCE_DIR}/tests/testStatsdClient.cpp)
target_link_libraries(testStatsdClient ${CMAKE_THREAD_LIBS_INIT})

enable_testing()
add_test(testSuite testStatsdClient)