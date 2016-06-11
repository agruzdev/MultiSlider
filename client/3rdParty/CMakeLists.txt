
project(Jsonxx)

file(GLOB jsonxx_source ./jsonxx/*.cc ./jsonxx/*.h)
add_library(Jsonxx STATIC ${jsonxx_source})

list(APPEND THIRD_PARTY_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/jsonxx")
list(APPEND THIRD_PARTY_TARGETS Jsonxx)

#------------------------------------------------------------
set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} PARENT_SCOPE)
set(THIRD_PARTY_TARGETS  ${THIRD_PARTY_TARGETS}  PARENT_SCOPE)
