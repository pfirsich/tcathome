#include "vm.hpp"

#include <string>

#include <fmt/core.h>

#include "fswatcher.hpp"
#include "memtrack.hpp"

std::string_view Vm::to_string(Mode mode)
{
    switch (mode) {
    case Mode::Advance:
        return "Advance";
    case Mode::Pause:
        return "Pause";
    case Mode::Playback:
        return "Playback";
    case Mode::Replay:
        return "Replay";
    default:
        return "UNKNOWN";
    }
}

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
    stop_timestamp.reset();
}

void Vm::seek_timestamp(uint64_t ts)
{
    mode = Mode::Pause;
    const auto frame_id = static_cast<uint32_t>(ts >> 32);
    memtrack::restore(frame_id);
    current_frame = frame_id;
    stop_timestamp = ts;
    gamecode::update(engine_state.game_code, state, engine_state.time, engine_state.dt);
    // Do not reset stop_timestamp, because the stop timestamp might be in render!
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

uint64_t Vm::next_timestamp()
{
    return (static_cast<uint64_t>(current_frame) << 32) | ++next_timestamp_id;
}

bool Vm::update_advance(const platform::InputState& input_state, float dt)
{
    assert(mode == Mode::Advance);
    set_input_state(input_state);
    copy_most_recent_hot_to_current();
    update_time(dt);
    return update();
}

void Vm::finish_frame_advance()
{
    save_next_frame();
}

bool Vm::update_replay(float dt)
{
    assert(mode == Mode::Replay);
    // Just restore input state. We restored everything else when we started the replay and
    // everything else depends on the new update code.
    copy_input_state(current_frame);
    update_time(dt);
    return update();
}

void Vm::finish_frame_replay()
{
    assert(mode == Mode::Replay);

    memtrack::overwrite(current_frame);

    if (current_frame == last_frame) {
        mode = Vm::Mode::Pause;
    } else {
        current_frame += 1;
    }
}

bool Vm::update_playback()
{
    assert(mode == Mode::Playback);
    memtrack::restore(current_frame);
    // Update so we lay sounds and stuff
    return update();
}

void Vm::finish_frame_playback()
{
    assert(mode == Mode::Playback);

    if (current_frame == last_frame) {
        mode = Vm::Mode::Pause;
    } else {
        current_frame += 1;
    }
}

void Vm::start_advance()
{
    mode = Vm::Mode::Advance;
    replay_mark.reset();
    stop_timestamp.reset();
}

void Vm::start_pause()
{
    mode = Vm::Mode::Pause;
    fmt::println("pause");
}

void Vm::start_replay(uint32_t start_frame_id)
{
    mode = Vm::Mode::Replay;
    seek(start_frame_id);
    // Overwrite the just restored state with the most recent code
    copy_most_recent_hot_to_current();
    stop_timestamp.reset();
}

void Vm::start_playback()
{
    mode = Vm::Mode::Playback;
    replay_mark.reset();
    stop_timestamp.reset();
}
