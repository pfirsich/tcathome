#include <optional>
#include <string>

#include "imgui.h"
#include <fmt/core.h>

#include "engine.hpp"
#include "fswatcher.hpp"
#include "memtrack.hpp"
#include "vm.hpp"

void show_state_inspector(const void* state);

void show_overlay(
    Vm::Mode mode, uint32_t current_frame, uint32_t last_frame, std::optional<uint32_t> replay_mark)
{
    if (mode == Vm::Mode::Advance) {
        return;
    }

    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 window_pos = { work_pos.x + PAD, work_pos.y + PAD };
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2 { 0.0f, 0.0f });

    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("overlay", nullptr, window_flags)) {
        ImGui::Text("Mode: %s", Vm::to_string(mode).data());
        ImGui::Text("Current Frame: %u", current_frame);
        ImGui::Text("Last Frame: %u", last_frame);
        if (replay_mark) {
            ImGui::Text("Replay Mark: %u", *replay_mark);
        } else {
            ImGui::Text("Replay Mark: none");
        }

        ImGui::Separator();
        if (mode == Vm::Mode::Pause) {
            if (current_frame == last_frame) {
                ImGui::Text("n: advance one frame");
            } else {
                ImGui::Text("n: skip 1 frame forwards");
            }
            ImGui::Text("shift+n: skip 20 frames forwards");
            ImGui::Text("p: skip 1 frame backwards");
            ImGui::Text("shift+p: skip 20 frames backwards");
            ImGui::Text("e: skip to last frame");
            ImGui::Text("c: continue from current frame");
            ImGui::Text("r: replay from mark");
            ImGui::Text("ctrl+r: replay from current frame");
            if (replay_mark) {
                ImGui::Text("ctrl+m: jump to replay mark");
            }
            if (replay_mark == current_frame) {
                ImGui::Text("m: unset replay mark");
            } else {
                ImGui::Text("m: set replay mark");
            }
            ImGui::Text("return: playback from here");
        } else if (mode == Vm::Mode::Playback) {
            ImGui::Text("space: pause");
        } else if (mode == Vm::Mode::Replay) {
            ImGui::Text("space: pause");
            if (replay_mark) {
                ImGui::Text("r: replay from mark");
            }
        }
        if (replay_mark) {
            ImGui::Text("reload file: replay from mark");
        }
    }
    ImGui::End();
}

int main(int, char**)
{
    platform::init("Game VM", 960, 1080);
    gfx::init();

    Vm vm;
    set_ng_vm(&vm);
    vm.init("game/game.c");

    platform::InputState input_state;

    auto replay = [&](uint32_t start_frame) {
        vm.mode = Vm::Mode::Replay;
        vm.seek(start_frame);
        // Overwrite the just restored state with the most recent code
        vm.copy_most_recent_hot_to_current();
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

        if (vm.replay_mark && reloaded) {
            replay(*vm.replay_mark);
            fmt::println("replay on reload");
        }

        if (vm.mode == Vm::Mode::Advance) {
            vm.set_input_state(input_state);
            vm.copy_most_recent_hot_to_current();
            vm.update_time(dt);

            const auto update_broken = vm.update();

            gfx::render_begin();
            const auto render_broken = vm.render();
            gfx::render_end();

            vm.save_next_frame();

            const auto broken = update_broken || render_broken;
            if (broken) {
                vm.mode = Vm::Mode::Pause;
                vm.replay_mark = vm.current_frame;
                fmt::println("break");
            }
        } else if (vm.mode == Vm::Mode::Pause) {
            // Do NOT update, so we don't play sounds or something while seeking

            // Just render and handle input (later)
            gfx::render_begin();
            vm.render();
            show_overlay(vm.mode, vm.current_frame, vm.last_frame, vm.replay_mark);
            show_state_inspector(vm.state);
            // ImGui::ShowDemoWindow();
            gfx::render_end();
        } else if (vm.mode == Vm::Mode::Playback) {
            memtrack::restore(vm.current_frame);

            // Do update so we play sound and stuff
            vm.update();

            gfx::render_begin();
            vm.render();
            show_overlay(vm.mode, vm.current_frame, vm.last_frame, vm.replay_mark);
            show_state_inspector(vm.state);
            gfx::render_end();

            if (vm.current_frame == vm.last_frame) {
                vm.mode = Vm::Mode::Pause;
            } else {
                vm.current_frame += 1;
            }
        } else if (vm.mode == Vm::Mode::Replay) {
            // Just restore input state. We restored everything else when we started the replay and
            // everything else depends on the new update code.
            vm.copy_input_state(vm.current_frame);
            vm.update_time(dt);

            vm.update();

            gfx::render_begin();
            vm.render();
            show_overlay(vm.mode, vm.current_frame, vm.last_frame, vm.replay_mark);
            show_state_inspector(vm.state);
            gfx::render_end();

            memtrack::overwrite(vm.current_frame);

            if (vm.current_frame == vm.last_frame) {
                vm.mode = Vm::Mode::Pause;
            } else {
                vm.current_frame += 1;
            }
        }

        // Handle input
        const auto ctrl = input_state.is_down("left ctrl") || input_state.is_down("right ctrl");
        const auto shift = input_state.is_down("left shift") || input_state.is_down("right shift");
        if (vm.mode == Vm::Mode::Advance) {
            if (input_state.is_pressed("return")) {
                vm.mode = Vm::Mode::Pause;
                fmt::println("pause");
            }
        } else if (vm.mode == Vm::Mode::Pause) {
            const uint32_t step = shift ? 20 : 1;
            if (input_state.is_pressed("n")) {
                // skip forward
                if (vm.current_frame == vm.last_frame) {
                    // advance one with last inputs
                    // We have restored the engine state from last_frame above
                    // (in the Pause update), so we will kepe the last inputs.
                    // We also keep the last code, but you can replay.
                    vm.update_time(dt);
                    vm.update();
                    vm.save_next_frame();
                    fmt::println("step");
                } else {
                    vm.seek(std::min(vm.current_frame + step, vm.last_frame));
                    fmt::println("current frame: {}", vm.current_frame);
                }
            } else if (input_state.is_pressed("p")) {
                // skip backwards
                vm.seek(std::max(step, vm.current_frame) - step);
                fmt::println("current frame: {}", vm.current_frame);
            } else if (input_state.is_pressed("e")) {
                // skip to last frame
                vm.current_frame = vm.last_frame;
                fmt::println("current frame: {}", vm.current_frame);
            } else if (input_state.is_pressed("c")) {
                // continue from here
                vm.mode = Vm::Mode::Advance;
                vm.replay_mark.reset();
                fmt::println("continue from frame {}", vm.current_frame);
            } else if (input_state.is_pressed("r")) {
                // replay
                if (ctrl) {
                    // replay from here
                    replay(vm.current_frame);
                    fmt::println("replay from {}", vm.current_frame);
                } else if (vm.replay_mark) {
                    // replay from mark or current frame if none is set
                    replay(*vm.replay_mark);
                    fmt::println("replay from {}", *vm.replay_mark);
                }
            } else if (ctrl && input_state.is_pressed("m") && vm.replay_mark) {
                // jump to mark
                vm.current_frame = *vm.replay_mark;
                fmt::println("current frame: {}", vm.current_frame);
            } else if (input_state.is_pressed("m")) {
                // mark / unmark
                if (vm.replay_mark == vm.current_frame) {
                    vm.replay_mark.reset();
                    fmt::println("unset mark");
                } else {
                    vm.replay_mark = vm.current_frame;
                    fmt::println("mark {}", vm.current_frame);
                }
            } else if (input_state.is_pressed("return")) {
                // playback from here
                vm.mode = Vm::Mode::Playback;
                vm.replay_mark.reset();
            }
        } else if (vm.mode == Vm::Mode::Playback) {
            if (input_state.is_pressed("space")) {
                vm.mode = Vm::Mode::Pause;
                fmt::println("pause");
            }
        } else if (vm.mode == Vm::Mode::Replay) {
            if (input_state.is_pressed("r") && vm.replay_mark) {
                replay(*vm.replay_mark);
            } else if (input_state.is_pressed("space")) {
                vm.mode = Vm::Mode::Pause;
                fmt::println("pause");
            }
        }
    }

    platform::shutdown();
}
