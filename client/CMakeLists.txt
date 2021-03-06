cmake_minimum_required (VERSION 3.5)

project(MultiSlider)

#--------------------------------
# Boost Asio

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ROOT $ENV{BOOST_ROOT})
find_package(Boost REQUIRED COMPONENTS system date_time regex)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

#--------------------------------
# 3rdParty projects

add_subdirectory(./3rdParty)
include_directories(${THIRD_PARTY_INCLUDES})

#--------------------------------

option(BUILD_TESTS "Build tests target" ON)
if(BUILD_TESTS)
   add_subdirectory(./tests)
endif()

#--------------------------------

add_definitions("/D_MULTISLIDER_EXPORT_DLL")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SCL_SECURE_NO_WARNINGS")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

file(GLOB all_sources ./source/*.h ./source/*.cpp)

set(CMAKE_DEBUG_POSTFIX "_d")

add_library(MultiSlider SHARED ${all_sources})

target_link_libraries(MultiSlider PRIVATE ${THIRD_PARTY_TARGETS})
target_link_libraries(MultiSlider PRIVATE ${Boost_LIBRARIES})
