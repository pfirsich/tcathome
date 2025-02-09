#pragma once

#include <array>
#include <type_traits>

#include "core.hpp"
#include "random.hpp"

struct Texture;

struct EngineState {
    platform::InputState input_state;
    rng::RandomState random_state;
    // This provides a mapping from image handle to texture
    std::array<gfx::Texture*, 16> textures = {};
};
static_assert(std::is_trivially_copyable_v<EngineState>);

void set_engine_state(EngineState* state);

// These functions will be called by the game, the state they implicitly reference is encapsulated
// by EngineState above and can be pointed to by set_engine_state;
extern "C" void* ng_alloc(size_t size);
extern "C" uint32_t ng_load_image(const char* path);
extern "C" void ng_draw_sprite(uint32_t image_handle, float x, float y);
extern "C" bool ng_is_key_down(const char* key);
extern "C" float ng_randomf();
