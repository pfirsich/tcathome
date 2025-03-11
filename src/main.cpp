#include <optional>
#include <string>

#include "imgui.h"
#include <fmt/core.h>

#include "engine.hpp"
#include "fswatcher.hpp"
#include "memtrack.hpp"
#include "vm.hpp"

void show_state_inspector(Vm* vm);
void show_overlay(const Vm* vm);

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
            show_overlay(&vm);
            show_state_inspector(&vm);
            // ImGui::ShowDemoWindow();
            gfx::render_end();
        } else if (vm.mode == Vm::Mode::Playback) {
            memtrack::restore(vm.current_frame);

            // Do update so we play sound and stuff
            vm.update();

            gfx::render_begin();
            vm.render();
            show_overlay(&vm);
            show_state_inspector(&vm);
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
            show_overlay(&vm);
            show_state_inspector(&vm);
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
