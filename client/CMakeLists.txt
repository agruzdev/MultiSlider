cmake_minimum_required (VERSION 3.1)

project(MultiSlider)

#--------------------------------
# RakNet

set(RAKNET_INCLUDE_DIR "" CACHE PATH "RakNet include dir")
include_directories(${RAKNET_INCLUDE_DIR})

set(RAKNET_LIBRARY_DIR "" CACHE PATH "RakNet library dir")
link_directories(${RAKNET_LIBRARY_DIR})

#--------------------------------
# 3rdParty projects

add_subdirectory(./3rdParty)
include_directories(${THIRD_PARTY_INCLUDES})

#--------------------------------

add_definitions("-D_MULTISLIDER_EXPORT_DLL")

file(GLOB all_sources ./source/*.h ./source/*.cpp)

add_library(MultiSlider SHARED ${all_sources})

target_link_libraries(MultiSlider PRIVATE RakNetDLL)
target_link_libraries(MultiSlider PRIVATE ${THIRD_PARTY_TARGETS})