#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

// This should be SDL_NUM_SCANCODES, but I don't want to include SDL just for this
constexpr size_t MaxNumScancodes = 512;

namespace platform {
void init(const char* title, uint32_t xres, uint32_t yres);
void shutdown();

struct InputState {
    // Index is Scancode
    std::array<uint8_t, MaxNumScancodes> keyboard_state = {};
    std::array<uint8_t, MaxNumScancodes> keyboard_pressed = {};
    std::array<uint8_t, MaxNumScancodes> keyboard_released = {};

    bool is_down(const char* key);
    bool is_pressed(const char* key);
};

int get_scancode(const char* name);

// returns whether the window is still open
bool process_events(InputState* state);

float get_time();

uint64_t get_perf_counter();
uint64_t get_perf_counter_freq();
float get_perf_counter_elapsed(uint64_t start, uint64_t factor);
}

namespace gfx {
void init();
void render_begin();
void render_end();

struct Texture;

Texture* load_texture(std::string_view path);
void draw(
    const Texture* texture, float x, float y, float scale, float r, float g, float b, float a);

}

template <typename T>
void copy_obj(T* dst, const T* src)
{
    std::memcpy(dst, src, sizeof(T));
}
