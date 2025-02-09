#include "core.hpp"

int main(int, char**)
{
    platform::init("Game VM", 1600, 900);
    gfx::init();

    platform::InputState input_state;
    float time = glwx::getTime();
    while (platform::process_events(&input_state)) {
        const auto now = glwx::getTime();
        const auto dt = now - time;
        time = now;

        gfx::render_begin();
        gfx::render_end();
    }
}
