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

add_executable(gvm ${GVM_SOURCES})
target_link_libraries(gvm PRIVATE glwx)
target_link_libraries(gvm PRIVATE tcc)
gvm_set_wall(gvm)
set_no_exceptions(gvm)
set_no_rtti(gvm)
