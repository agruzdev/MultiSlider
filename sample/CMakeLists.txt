cmake_minimum_required (VERSION 3.1)

project(MultiSliderSamples)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build library
add_subdirectory(../client ${CMAKE_BINARY_DIR}/MultiSlider)

# Build samples
add_subdirectory(source/ApiTest     ${CMAKE_BINARY_DIR}/ApiTest)
add_subdirectory(source/TickTackToe ${CMAKE_BINARY_DIR}/TickTackToe)