cmake_minimum_required (VERSION 3.1)

project(TickTackToe)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(TickTackToe Main.cpp)

target_link_libraries(TickTackToe MultiSlider)