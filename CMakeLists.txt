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
  src/core.cpp
  src/engine.cpp
  src/fswatcher.cpp
  src/gamecode.cpp
  src/gui.cpp
  src/main.cpp
  src/memtrack.cpp
  src/random.cpp
  src/vm.cpp
)

set(IMGUI_SOURCES
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/imgui_stdlib.cpp
  deps/imgui/imgui_tables.cpp
  deps/imgui/imgui_widgets.cpp

  deps/imgui/imgui_impl_opengl3.cpp
  deps/imgui/imgui_impl_sdl2.cpp
)

add_executable(gvm ${GVM_SOURCES} ${IMGUI_SOURCES})
target_include_directories(gvm PRIVATE deps/imgui)
target_link_libraries(gvm PRIVATE glwx)
target_link_libraries(gvm PRIVATE tcc)
gvm_set_wall(gvm)
set_no_exceptions(gvm)
set_no_rtti(gvm)
