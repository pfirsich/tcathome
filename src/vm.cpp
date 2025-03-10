#include "vm.hpp"

#include <string>

#include <fmt/core.h>

#include "fswatcher.hpp"
#include "memtrack.hpp"

static void reload_game_code(void* ctx, std::string_view path)
{
    auto vm = (Vm*)ctx;
    auto gc = gamecode::load(std::string(path).c_str());
    if (!gc) {
        fmt::println("Could not reload game code");
        return;
    }
    fmt::println("New game code: {}", fmt::ptr(gc));
    vm->engine_state.game_code = gc;
}

void Vm::init(const char* game_source)
{
    engine_state_track = memtrack::track(&engine_state, sizeof(EngineState));
    rng::init_state(&engine_state.random_state);

    engine_state.game_code = gamecode::load(game_source);
    fsw::add_watch(game_source, reload_game_code, this);
    state = gamecode::load(engine_state.game_code);
    copy_obj(&hot_most_recent, static_cast<HotReloadState*>(&engine_state));

    save_next_frame();
}

bool Vm::update()
{
    return gamecode::update(engine_state.game_code, state, engine_state.time, engine_state.dt);
}

bool Vm::render()
{
    return gamecode::render(engine_state.game_code, state);
}

void Vm::update_time(float dt)
{
    engine_state.time += dt;
    engine_state.dt = dt;
}

void Vm::seek(uint32_t frame_id)
{
    memtrack::restore(frame_id);
    current_frame = frame_id;
}

void Vm::copy_most_recent_hot_to_current()
{
    copy_obj(static_cast<HotReloadState*>(&engine_state), &hot_most_recent);
}

void Vm::save_next_frame()
{
    current_frame = memtrack::save();
    last_frame = current_frame;
}

void Vm::set_input_state(const platform::InputState& state)
{
    copy_obj(&engine_state.input_state, &state);
}

void Vm::copy_input_state(uint32_t source_frame_id)
{
    memtrack::restore_to(engine_state_track, source_frame_id,
        (uintptr_t)&engine_state.input_state - (uintptr_t)&engine_state,
        sizeof(platform::InputState), &engine_state.input_state);
}
