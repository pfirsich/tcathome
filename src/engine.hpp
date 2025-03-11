#pragma once

#include "vm.hpp"

// Store a pointer to the VM instance to be referenced by the ng functions below
void set_ng_vm(Vm* vm);

// These functions will be called by the game, the state they implicitly reference is encapsulated
// by EngineState above and can be pointed to by set_engine_state;
extern "C" void* ng_alloc(size_t size);
extern "C" uint32_t ng_load_image(const char* path);
extern "C" void ng_draw_sprite(
    uint32_t image_handle, float x, float y, float scale, float r, float g, float b, float a);
extern "C" bool ng_is_key_down(const char* key);
extern "C" int ng_key_pressed(const char* key);
extern "C" float ng_randomf();
extern "C" void ng_break_internal(const char* file, int line);
extern "C" uint64_t ng_timestamp_internal(const char* file, int line);
extern "C" void ng_error_internal(const char* file, int line, const char* message);
