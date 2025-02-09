#include "engine.hpp"

EngineState* engine_state;

void set_engine_state(EngineState* state) {
    engine_state = state;
}

extern "C" uint32_t ng_load_image(const char* path)
{
    uint32_t idx = 0;
    while(idx < engine_state->textures.size() && engine_state->textures[idx] != nullptr) {
        idx++;
    }
    assert(idx < engine_state->textures.size());
    engine_state->textures[idx] = gfx::load_texture(path);
    return idx + 1; // 0 is invalid
}

extern "C" void ng_draw_sprite(uint32_t image_handle, float x, float y)
{
    assert(image_handle != 0 && image_handle <= engine_state->textures.size());
    const auto texture = engine_state->textures[image_handle - 1];
    gfx::draw(texture, x, y);
}

extern "C" bool ng_is_key_down(const char* key)
{
    const auto sc = platform::get_scancode(key);
    assert(sc > 0 && static_cast<size_t>(sc) < MaxNumScancodes);
    return engine_state->input_state.keyboard_state[sc] > 0;
}

extern "C" float ng_randomf()
{
    return rng::randomf(&engine_state->random_state);
}
