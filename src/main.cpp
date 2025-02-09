#include "core.hpp"
#include "engine.hpp"
#include "random.hpp"

int main(int, char**)
{
    platform::init("Game VM", 1600, 900);
    gfx::init();

    EngineState engine_state;
    rng::init_state(&engine_state.random_state);
    set_engine_state(&engine_state);

    const auto sprite = ng_load_image("assets/croc.png");

    const auto x = 200.0f + ng_randomf() * 1200.0f;
    const auto y = 200.0f + ng_randomf() * 500.0f;

    float time = platform::get_time();
    while (platform::process_events(&engine_state.input_state)) {
        const auto now = platform::get_time();
        const auto dt = now - time;
        time = now;

        gfx::render_begin();
        ng_draw_sprite(sprite, x, y);
        gfx::render_end();
    }
}
