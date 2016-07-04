cmake_minimum_required (VERSION 3.1)

project(ApiTest)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(ApiTest Main.cpp)

target_link_libraries(ApiTest MultiSlider)