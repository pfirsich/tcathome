#include <optional>
#include <string>

#include "core.hpp"
#include "engine.hpp"
#include "fswatcher.hpp"
#include "gamecode.hpp"
#include "memtrack.hpp"
#include "random.hpp"

#include <fmt/core.h>

void reload_game_code(void* ctx, std::string_view path)
{
    auto engine_state = (EngineState*)ctx;
    auto gc = gamecode::load(std::string(path).c_str());
    if (!gc) {
        fmt::println("Could not reload game code");
        return;
    }
    engine_state->game_code = gc;
}

bool key_pressed(std::array<uint8_t, MaxNumScancodes>& pressed, const char* key)
{
    return pressed[static_cast<size_t>(platform::get_scancode(key))];
}

int main(int, char**)
{
    platform::init("Game VM", 960, 1080);
    gfx::init();

    EngineState engine_state;
    memtrack::track(&engine_state, sizeof(EngineState));
    set_engine_state(&engine_state);

    rng::init_state(&engine_state.random_state);
    engine_state.game_code = gamecode::load("game/game.c");
    fsw::add_watch("game/game.c", reload_game_code, &engine_state);
    auto state = gamecode::load(engine_state.game_code);

    auto last_frame_id = memtrack::save();
    std::optional<uint32_t> replay_frame;
    std::array<uint8_t, MaxNumScancodes> pressed;

    float time = platform::get_time();
    while (platform::process_events(&engine_state.input_state)) {
        // We need to back it up here, because memtrack::restore might replace it later
        std::memcpy(
            pressed.data(), engine_state.input_state.keyboard_pressed.data(), MaxNumScancodes);

        const auto now = platform::get_time();
        const auto dt = now - time;
        time = now;

        if (!replay_frame) {
            // regular advance
            fsw::update();

            const auto update_broken = gamecode::update(engine_state.game_code, state, time, dt);

            gfx::render_begin();
            const auto render_broken = gamecode::render(engine_state.game_code, state);
            gfx::render_end();

            last_frame_id = memtrack::save();

            const auto broken = update_broken || render_broken;
            if (broken || key_pressed(pressed, "f10")) {
                replay_frame = last_frame_id;
                fmt::println("replay {}", *replay_frame);
            }
        } else {
            memtrack::restore(*replay_frame);

            fsw::update();

            gamecode::update(engine_state.game_code, state, time, dt);

            gfx::render_begin();
            gamecode::render(engine_state.game_code, state);
            gfx::render_end();

            static constexpr uint32_t step = 1;
            if (key_pressed(pressed, "n")) {
                *replay_frame = std::min(*replay_frame + step, last_frame_id);
                fmt::println("replay {}", *replay_frame);
            } else if (key_pressed(pressed, "p")) {
                *replay_frame = std::max(*replay_frame, step) - step;
                fmt::println("replay {}", *replay_frame);
            } else if (key_pressed(pressed, "c")) {
                fmt::println("replay {}", *replay_frame);
                replay_frame.reset();
                memtrack::restore(last_frame_id);
                fmt::println("continue");
            }
        }
    }
}
