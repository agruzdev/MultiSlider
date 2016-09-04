project(MultiSliderTests)

#==========================================
# Dependencies

# Download and unpack googletest at configure time
configure_file(./gtest/CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
#execute_process(COMMAND ${CMAKE_COMMAND} --build .
#  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
 
# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
 
# Add googletest directly to our build. This adds
# the following targets: gtest, gtest_main, gmock
# and gmock_main
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build)
 
# The gtest/gmock targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include"
                      "${gmock_SOURCE_DIR}/include")
endif()
 
# Now simply link your own targets against gtest, gmock,
# etc. as appropriate

#==========================================
# Source

#ExternalProject_Get_Property(GTest source_dir)
#set(GTEST_INCLUDE_DIR ${source_dir}/include)

file(GLOB all_sources ./source/*.cpp ./source/*.h)


    
#==========================================
# Target
add_executable(MultiSliderTests ${all_sources})

target_link_libraries(MultiSliderTests MultiSlider)
target_link_libraries(MultiSliderTests gtest)
target_link_libraries(MultiSliderTests gtest_main)