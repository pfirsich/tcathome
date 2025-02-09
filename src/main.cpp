#include <string>

#include "core.hpp"
#include "engine.hpp"
#include "fswatcher.hpp"
#include "random.hpp"
#include "gamecode.hpp"

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

int main(int, char**)
{
    platform::init("Game VM", 1600, 900);
    gfx::init();

    EngineState engine_state;
    set_engine_state(&engine_state);

    rng::init_state(&engine_state.random_state);
    engine_state.game_code = gamecode::load("game/game.c");
    fsw::add_watch("game/game.c", reload_game_code, &engine_state);
    auto state = gamecode::load(engine_state.game_code);

    float time = platform::get_time();
    while (platform::process_events(&engine_state.input_state)) {
        const auto now = platform::get_time();
        const auto dt = now - time;
        time = now;

        fsw::update();

        gamecode::update(engine_state.game_code, state, time, dt);

        gfx::render_begin();
        gamecode::render(engine_state.game_code, state);
        gfx::render_end();
    }
}
