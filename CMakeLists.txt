cmake_minimum_required(VERSION 3.3.0)

project(gvm)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(FetchContent)

include(cmake/wall.cmake)
include(cmake/no-rtti.cmake)
include(cmake/no-exceptions.cmake)

if(GVM_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

include(cmake/tinycc.cmake)

add_subdirectory(deps/glwrap)

set(GVM_SOURCES
  src/main.cpp
  src/core.cpp
  src/engine.cpp
  src/random.cpp
  src/fswatcher.cpp
  src/gamecode.cpp
)

add_executable(gvm ${GVM_SOURCES})
target_link_libraries(gvm PRIVATE glwx)
target_link_libraries(gvm PRIVATE tcc)
gvm_set_wall(gvm)
set_no_exceptions(gvm)
set_no_rtti(gvm)
