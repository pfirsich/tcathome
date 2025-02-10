#include "engine.hpp"

#include "fswatcher.hpp"
#include "memtrack.hpp"

#include <fmt/core.h>

EngineState* engine_state;

void set_engine_state(EngineState* state)
{
    engine_state = state;
}

void reload_image(void* ctx, std::string_view path)
{
    const auto idx = (uintptr_t)ctx;
    const auto tex = gfx::load_texture(path);
    if (!tex) {
        fmt::println("Could not reload {}", path);
        return;
    }
    engine_state->textures[idx] = tex;
}

extern "C" void* ng_alloc(size_t size)
{
    auto ptr = malloc(size);
    memtrack::track(ptr, size);
    return ptr;
}

extern "C" uint32_t ng_load_image(const char* path)
{
    uint32_t idx = 0;
    while (idx < engine_state->textures.size() && engine_state->textures[idx] != nullptr) {
        idx++;
    }
    assert(idx < engine_state->textures.size());
    engine_state->textures[idx] = gfx::load_texture(path);
    assert(engine_state->textures[idx]);
    fsw::add_watch(path, reload_image, (void*)static_cast<uintptr_t>(idx));
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

extern "C" void ng_break()
{
    gamecode::ng_break();
}

extern "C" void ng_break_if(bool cond)
{
    if (cond) {
        ng_break();
    }
}
