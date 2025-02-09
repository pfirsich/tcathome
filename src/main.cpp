#include "core.hpp"

int main(int, char**) {
    platform::init("Game VM", 1600, 900);
    gfx::init();

    platform::InputState input_state;
    while(platform::process_events(&input_state)) {
        gfx::render_begin();
        gfx::render_end();
    }
}
