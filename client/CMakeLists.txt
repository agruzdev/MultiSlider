cmake_minimum_required (VERSION 3.1)

project(MultiSlider)

#--------------------------------
# RakNet

set(RAKNET_INCLUDE_DIR "" CACHE PATH "RakNet include dir")
include_directories(${RAKNET_INCLUDE_DIR})

set(RAKNET_LIBRARY_DIR "" CACHE PATH "RakNet library dir")
link_directories(${RAKNET_LIBRARY_DIR})

#--------------------------------
# ENET

set(ENET_INCLUDE_DIR "" CACHE PATH "ENET include dir")
include_directories(${ENET_INCLUDE_DIR})

set(ENET_LIBRARY_DIR "" CACHE PATH "ENET library dir")
link_directories(${ENET_LIBRARY_DIR})

#--------------------------------
# 3rdParty projects

add_subdirectory(./3rdParty)
include_directories(${THIRD_PARTY_INCLUDES})

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

add_library(MultiSlider SHARED ${all_sources})

target_link_libraries(MultiSlider PRIVATE RakNetDLL)
target_link_libraries(MultiSlider PRIVATE ${THIRD_PARTY_TARGETS})
target_link_libraries(MultiSlider PRIVATE enet)