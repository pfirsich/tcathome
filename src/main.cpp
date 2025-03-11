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
void show_error(const Vm::Error& error);

void render_debug(Vm* vm)
{
    gfx::render_begin();
    vm->render();
    show_overlay(vm);
    show_state_inspector(vm);
    // ImGui::ShowDemoWindow();
    if (vm->error) {
        show_error(*vm->error);
    }
    gfx::render_end();
}

int main(int, char**)
{
    platform::init("Game VM", 960, 1080);
    gfx::init();

    Vm vm;
    set_ng_vm(&vm);
    vm.init("game/game.c");

    platform::InputState input_state;

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
            vm.start_replay(*vm.replay_mark);
            fmt::println("replay on reload");
        }

        if (vm.mode == Vm::Mode::Advance) {
            const auto update_broken = vm.update_advance(input_state, dt);

            gfx::render_begin();
            const auto render_broken = vm.render();
            gfx::render_end();

            vm.finish_frame_advance();

            const auto broken = update_broken || render_broken;
            if (broken) {
                vm.start_pause();
                vm.replay_mark = vm.current_frame;
                fmt::println("break");
            }
        } else if (vm.mode == Vm::Mode::Pause) {
            // Do NOT update, so we don't play sounds or something while seeking

            // Just render and handle input (later)
            render_debug(&vm);
        } else if (vm.mode == Vm::Mode::Playback) {
            vm.update_playback();

            render_debug(&vm);

            vm.finish_frame_playback();
        } else if (vm.mode == Vm::Mode::Replay) {
            vm.update_replay(dt);

            render_debug(&vm);

            vm.finish_frame_replay();
        }

        // Handle input
        const auto ctrl = input_state.is_down("left ctrl") || input_state.is_down("right ctrl");
        const auto shift = input_state.is_down("left shift") || input_state.is_down("right shift");
        if (vm.mode == Vm::Mode::Advance) {
            if (input_state.is_pressed("return")) {
                vm.start_pause();
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
                vm.start_advance();
                fmt::println("continue from frame {}", vm.current_frame);
            } else if (input_state.is_pressed("r")) {
                // replay
                if (ctrl) {
                    // replay from here
                    vm.start_replay(vm.current_frame);
                    fmt::println("replay from {}", vm.current_frame);
                } else if (vm.replay_mark) {
                    // replay from mark or current frame if none is set
                    vm.start_replay(*vm.replay_mark);
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
                vm.start_playback();
            }
        } else if (vm.mode == Vm::Mode::Playback) {
            if (input_state.is_pressed("space")) {
                vm.start_pause();
            }
        } else if (vm.mode == Vm::Mode::Replay) {
            if (input_state.is_pressed("r") && vm.replay_mark) {
                vm.start_replay(*vm.replay_mark);
            } else if (input_state.is_pressed("space")) {
                vm.start_pause();
            }
        }
    }

    platform::shutdown();
}
