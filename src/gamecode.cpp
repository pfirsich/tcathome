#include "gamecode.hpp"

#include <csetjmp>

#include <fmt/core.h>
#include <tcc.h>

#include "engine.hpp"

using LoadFunc = void*();
using UpdateFunc = void(void*, float t, float dt);
using RenderFunc = void(const void*);

struct GameCode {
    TCCState* tcc = nullptr;
    LoadFunc* load = nullptr;
    UpdateFunc* update = nullptr;
    RenderFunc* render = nullptr;
};

std::array<GameCode, 256> game_codes;
std::jmp_buf jump_buf;
bool in_callback = false;

namespace gamecode {
GameCode* load(const char* path)
{
    size_t i = 0;
    while (i < game_codes.size() && game_codes[i].tcc != nullptr) {
        i++;
    }
    assert(i < game_codes.size());
    auto& gc = game_codes[i];

    const auto start = platform::get_perf_counter();
    gc.tcc = tcc_new();
    assert(gc.tcc);

    tcc_set_output_type(gc.tcc, TCC_OUTPUT_MEMORY);

    if (tcc_add_file(gc.tcc, path) == -1) {
        fmt::println("compile failed");
        return nullptr;
    }

    // For runtime library libtcc1.
    // This is actually not fixed by tcc_set_lib_path, which would make sense.
    // Why is this runtime lib not already part of libtcc?
    tcc_add_library_path(gc.tcc, "build/tinycc/");

    tcc_add_symbol(gc.tcc, "ng_alloc", (const void*)ng_alloc);
    tcc_add_symbol(gc.tcc, "ng_load_image", (const void*)ng_load_image);
    tcc_add_symbol(gc.tcc, "ng_draw_sprite", (const void*)ng_draw_sprite);
    tcc_add_symbol(gc.tcc, "ng_is_key_down", (const void*)ng_is_key_down);
    tcc_add_symbol(gc.tcc, "ng_random_float", (const void*)ng_randomf);

    if (tcc_relocate(gc.tcc) < 0) {
        fmt::println("relocate failed");
        return nullptr;
    }

    gc.load = (LoadFunc*)tcc_get_symbol(gc.tcc, "load");
    gc.update = (UpdateFunc*)tcc_get_symbol(gc.tcc, "update");
    gc.render = (RenderFunc*)tcc_get_symbol(gc.tcc, "render");

    fmt::println(
        "compiled new code in {}us", platform::get_perf_counter_elapsed(start, 1000 * 1000));

    return &gc;
}

void* load(GameCode* gc)
{
    return gc->load();
}

bool update(GameCode* gc, void* state, float t, float dt)
{
    if (setjmp(jump_buf)) {
        return true;
    } else {
        in_callback = true;
        gc->update(state, t, dt);
        in_callback = false;
        return false;
    }
}

bool render(GameCode* gc, const void* state)
{
    if (setjmp(jump_buf)) {
        return true;
    } else {
        in_callback = true;
        gc->render(state);
        in_callback = false;
        return false;
    }
}

void ng_break()
{
    assert(in_callback);
    std::longjmp(jump_buf, 1);
}
}
