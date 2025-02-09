#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// This should be SDL_NUM_SCANCODES, but I don't want to include SDL just for this
constexpr size_t MaxNumScancodes = 512;

namespace platform {
void init(const char* title, uint32_t xres, uint32_t yres);

struct InputState {
    std::array<uint8_t, MaxNumScancodes> keyboard_state = {};
    std::array<uint8_t, MaxNumScancodes> keyboard_pressed = {};
    std::array<uint8_t, MaxNumScancodes> keyboard_released = {};
};

// returns whether the window is still open
bool process_events(InputState* state);
}

namespace gfx {
void init();
void render_begin();
void render_end();
}
