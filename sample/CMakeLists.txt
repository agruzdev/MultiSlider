cmake_minimum_required (VERSION 3.1)

project(MultiSliderSample)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB all_sources ./source/*.h ./source/*.cpp)

add_executable(MultiSliderSample ${all_sources})

add_subdirectory(../client ${CMAKE_BINARY_DIR}/MultiSlider)

target_link_libraries(MultiSliderSample MultiSlider)