#include "engine.hpp"

#include "fswatcher.hpp"
#include "memtrack.hpp"

#include <fmt/core.h>

Vm* vm;

void set_ng_vm(Vm* p)
{
    vm = p;
}

static void reload_image(void* ctx, std::string_view path)
{
    const auto idx = (uintptr_t)ctx;
    const auto tex = gfx::load_texture(path);
    if (!tex) {
        fmt::println("Could not reload {}", path);
        return;
    }
    vm->hot_most_recent.textures[idx] = tex;
}

extern "C" void* ng_alloc(size_t size)
{
    auto ptr = malloc(size);
    memset(ptr, 0, size);
    memtrack::track(ptr, size);
    return ptr;
}

extern "C" uint32_t ng_load_image(const char* path)
{
    uint32_t idx = 0;
    while (idx < vm->engine_state.textures.size() && vm->engine_state.textures[idx] != nullptr) {
        idx++;
    }
    assert(idx < vm->engine_state.textures.size());
    vm->engine_state.textures[idx] = gfx::load_texture(path);
    assert(vm->engine_state.textures[idx]);
    fsw::add_watch(path, reload_image, (void*)static_cast<uintptr_t>(idx));
    return idx + 1; // 0 is invalid
}

extern "C" void ng_draw_sprite(
    uint32_t image_handle, float x, float y, float scale, float r, float g, float b, float a)
{
    assert(image_handle != 0 && image_handle <= vm->engine_state.textures.size());
    const auto texture = vm->engine_state.textures[image_handle - 1];
    gfx::draw(texture, x, y, scale, r, g, b, a);
}

extern "C" bool ng_is_key_down(const char* key)
{
    return vm->engine_state.input_state.is_down(key);
}

extern "C" int ng_key_pressed(const char* key)
{
    return vm->engine_state.input_state.is_pressed(key);
}

extern "C" float ng_randomf()
{
    return rng::randomf(&vm->engine_state.random_state);
}

extern "C" void ng_break_internal(const char* file, int line)
{
    fmt::println("break from {}:{}", file, line);
    gamecode::ng_break();
}

extern "C" uint64_t ng_timestamp_internal(const char* file, int line)
{
    const auto ts = vm->next_timestamp();
    if (vm->stop_timestamp == ts) {
        ng_break_internal(file, line);
    }
    return ts;
}
