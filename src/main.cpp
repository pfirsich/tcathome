#include <optional>
#include <string>

#include "imgui.h"
#include <fmt/core.h>

#include "core.hpp"
#include "engine.hpp"
#include "fswatcher.hpp"
#include "gamecode.hpp"
#include "memtrack.hpp"
#include "random.hpp"

void reload_game_code(void* ctx, std::string_view path)
{
    auto engine_state = (EngineState*)ctx;
    auto gc = gamecode::load(std::string(path).c_str());
    if (!gc) {
        fmt::println("Could not reload game code");
        return;
    }
    fmt::println("New game code: {}", fmt::ptr(gc));
    engine_state->game_code = gc;
}

bool get_key(std::array<uint8_t, MaxNumScancodes>& state, const char* key)
{
    return state[static_cast<size_t>(platform::get_scancode(key))];
}

bool key_down(platform::InputState& state, const char* key)
{
    return get_key(state.keyboard_state, key);
}

bool key_pressed(platform::InputState& state, const char* key)
{
    return get_key(state.keyboard_pressed, key);
}

bool render(GameCode* game_code, void* state)
{
    gfx::render_begin();
    const auto broken = gamecode::render(game_code, state);
    gfx::render_end();
    return broken;
}

enum class Mode { Advance, Pause, Playback, Replay };

std::string_view to_string(Mode mode)
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

void show_overlay(
    Mode mode, uint32_t current_frame, uint32_t last_frame, std::optional<uint32_t> replay_mark)
{
    static int location = 0;
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    assert(location >= 0 && location <= 4);
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("overlay", nullptr, window_flags)) {
        ImGui::Text(fmt::format("Mode: {}", to_string(mode)).c_str());
        ImGui::Text(fmt::format("Current Frame: {}", current_frame).c_str());
        ImGui::Text(fmt::format("Last Frame: {}", last_frame).c_str());
        if (replay_mark) {
            ImGui::Text(fmt::format("Replay Mark: {}", *replay_mark).c_str());
        } else {
            ImGui::Text("Replay Mark: none");
        }
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Top-left", NULL, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3))
                location = 3;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

int main(int, char**)
{
    platform::init("Game VM", 960, 1080);
    gfx::init();

    EngineState engine_state;
    const auto engine_state_track = memtrack::track(&engine_state, sizeof(EngineState));
    set_engine_state(&engine_state);
    HotReloadState hot_current;

    rng::init_state(&engine_state.random_state);
    engine_state.game_code = gamecode::load("game/game.c");
    fsw::add_watch("game/game.c", reload_game_code, &engine_state);
    auto state = gamecode::load(engine_state.game_code);
    std::memcpy(&hot_current, static_cast<HotReloadState*>(&engine_state), sizeof(HotReloadState));

    platform::InputState input_state;
    auto current_frame = memtrack::save();
    auto last_frame = current_frame;
    std::optional<uint32_t> replay_mark;
    Mode mode = Mode::Advance;

    auto seek = [&](uint32_t frame_id) {
        memtrack::restore(frame_id);
        current_frame = frame_id;
    };

    auto replay = [&](uint32_t start_frame) {
        mode = Mode::Replay;
        seek(start_frame);
        // Overwrite the just restored state with new code
        std::memcpy(
            static_cast<HotReloadState*>(&engine_state), &hot_current, sizeof(HotReloadState));
    };

    float time = platform::get_time();
    while (platform::process_events(&input_state)) {
        const auto now = platform::get_time();
        const auto dt = now - time;
        time = now;

        // We always want to reload. Even with we are pausing, we need to reload new code so replay
        // will do what it is supposed to. If we don't want wo to use the new code, we will restore
        // first anyways.
        const auto reloaded = fsw::update();
        if (reloaded) {
            // Keep a copy of the most current code and assets
            std::memcpy(
                &hot_current, static_cast<HotReloadState*>(&engine_state), sizeof(HotReloadState));
        }

        if (replay_mark && reloaded) {
            // replay from mark
            mode = Mode::Replay;
            replay(*replay_mark);
            fmt::println("replay on reload");
        }

        if (mode == Mode::Advance) {
            std::memcpy(&engine_state.input_state, &input_state, sizeof(platform::InputState));

            const auto update_broken = gamecode::update(engine_state.game_code, state, time, dt);
            const auto render_broken = render(engine_state.game_code, state);

            current_frame = memtrack::save();
            last_frame = current_frame;

            const auto broken = update_broken || render_broken;
            if (broken) {
                mode = Mode::Pause;
                replay_mark = current_frame;
                fmt::println("break");
            }
        } else if (mode == Mode::Pause) {
            // Do NOT update, so we don't play sounds or something while seeking

            // Just render and handle input (later)
            gfx::render_begin();
            gamecode::render(engine_state.game_code, state);
            show_overlay(mode, current_frame, last_frame, replay_mark);
            ImGui::ShowDemoWindow();
            gfx::render_end();
        } else if (mode == Mode::Playback) {
            memtrack::restore(current_frame);

            // Do update so we play sound and stuff
            gamecode::update(engine_state.game_code, state, time, dt);

            render(engine_state.game_code, state);

            if (current_frame == last_frame) {
                mode = Mode::Pause;
            } else {
                current_frame = current_frame + 1;
            }
        } else if (mode == Mode::Replay) {
            // Just restore input state. We restored everything else when we started the replay and
            // everything else depends on the new update code.
            memtrack::restore_to(engine_state_track, current_frame,
                (uintptr_t)&engine_state.input_state - (uintptr_t)&engine_state,
                sizeof(platform::InputState), &engine_state.input_state);

            gamecode::update(engine_state.game_code, state, time, dt);

            render(engine_state.game_code, state);

            memtrack::overwrite(current_frame);

            if (current_frame == last_frame) {
                mode = Mode::Pause;
            } else {
                current_frame = current_frame + 1;
            }
        }

        // Handle input
        const auto ctrl = key_down(input_state, "left ctrl") || key_down(input_state, "right ctrl");
        const auto shift
            = key_down(input_state, "left shift") || key_down(input_state, "right shift");
        if (mode == Mode::Advance) {
            if (key_pressed(input_state, "return")) {
                mode = Mode::Pause;
                current_frame = last_frame;
                fmt::println("pause");
            }
        } else if (mode == Mode::Pause) {
            const uint32_t step = shift ? 20 : 1;
            if (key_pressed(input_state, "n")) {
                // skip forward
                if (current_frame == last_frame) {
                    // advance one with last inputs
                    // We have restored the engine state from last_frame above
                    // (in the Pause update), so we will kepe the last inputs.
                    // We also keep the last code, but you can replay.
                    gamecode::update(engine_state.game_code, state, time, dt);
                    current_frame = memtrack::save();
                    last_frame = current_frame;
                    fmt::println("step");
                } else {
                    seek(std::min(current_frame + step, last_frame));
                    fmt::println("current frame: {}", current_frame);
                }
            } else if (key_pressed(input_state, "p")) {
                // skip backwards
                seek(std::max(step, current_frame) - step);
                fmt::println("current frame: {}", current_frame);
            } else if (key_pressed(input_state, "e")) {
                // skip to last frame
                current_frame = last_frame;
                fmt::println("current frame: {}", current_frame);
            } else if (key_pressed(input_state, "c")) {
                // continue from here
                mode = Mode::Advance;
                replay_mark.reset();
                fmt::println("continue from frame {}", current_frame);
            } else if (key_pressed(input_state, "r")) {
                // replay
                if (ctrl) {
                    // replay from here
                    replay(current_frame);
                } else {
                    // replay from mark or current frame if none is set
                    replay(replay_mark.value_or(current_frame));
                }
                fmt::println("replay from {}", current_frame);
            } else if (ctrl && key_pressed(input_state, "m") && replay_mark) {
                // jump to mark
                current_frame = *replay_mark;
                fmt::println("current frame: {}", current_frame);
            } else if (key_pressed(input_state, "m")) {
                // mark / unmark
                if (replay_mark == current_frame) {
                    replay_mark.reset();
                    fmt::println("unset mark");
                } else {
                    replay_mark = current_frame;
                    fmt::println("mark {}", current_frame);
                }
            } else if (key_pressed(input_state, "return")) {
                // playback from here
                mode = Mode::Playback;
                replay_mark.reset();
            }
        } else if (mode == Mode::Playback) {
            if (key_pressed(input_state, "space")) {
                mode = Mode::Pause;
                fmt::println("pause");
            }
        } else if (mode == Mode::Replay) {
            if (key_pressed(input_state, "r") && replay_mark) {
                replay(*replay_mark);
            } else if (key_pressed(input_state, "space")) {
                mode = Mode::Pause;
                fmt::println("pause");
            }
        }
    }

    platform::shutdown();
}
