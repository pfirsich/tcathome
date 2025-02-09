#include "core.hpp"

#include <memory>

#include <glwx/spriterenderer.hpp>
#include <glwx/window.hpp>

struct Platform {
    glwx::Window window;

    static Platform& instance()
    {
        static Platform plat;
        return plat;
    }
};

namespace platform {
    void init(const char* title, uint32_t xres, uint32_t yres) {
        auto& plat = Platform::instance();
        plat.window = glwx::makeWindow(title, xres, yres).value();
    }

    bool process_events(InputState* state) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return false;
                }
                break;
            case SDL_KEYUP:
                break;
            }
        }
        return true;
    }
}

struct Gfx {
    std::unique_ptr<glwx::SpriteRenderer> renderer;
    glm::mat4 projection;

    static Gfx& instance()
    {
        static Gfx gfx;
        return gfx;
    }
};

namespace gfx {
    void init() {
        auto& plat = Platform::instance();
        auto& gfx = Gfx::instance();
        glw::State::instance().setViewport(plat.window.getSize().x, plat.window.getSize().y);
        gfx.renderer = std::make_unique<glwx::SpriteRenderer>();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gfx.projection = glm::ortho(0.0f, static_cast<float>(plat.window.getSize().x),
            static_cast<float>(plat.window.getSize().y), 0.0f);
        glDepthFunc(GL_ALWAYS);
    }

    void render_begin() {
        auto& gfx = Gfx::instance();
        glClear(GL_COLOR_BUFFER_BIT);
        gfx.renderer->getDefaultShaderProgram().bind();
        gfx.renderer->getDefaultShaderProgram().setUniform("projectionMatrix", gfx.projection);
    }

    void render_end() {
        Gfx::instance().renderer->flush();
        Platform::instance().window.swap();
    }
}
