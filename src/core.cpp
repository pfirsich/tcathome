#include "core.hpp"
#include "glwx/texture.hpp"

#include <memory>

#include <glwx/spriterenderer.hpp>
#include <glwx/window.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

struct Platform {
    glwx::Window window;

    static Platform& instance()
    {
        static Platform plat;
        return plat;
    }
};

namespace platform {
void init(const char* title, uint32_t xres, uint32_t yres)
{
    auto& plat = Platform::instance();
    plat.window = glwx::makeWindow(title, xres, yres).value();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(plat.window.getSdlWindow(), plat.window.getSdlGlContext());
    ImGui_ImplOpenGL3_Init();
}

void shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

int get_scancode(const char* name)
{
    return SDL_GetScancodeFromName(name);
}

bool process_events(InputState* state)
{
    std::memset(state->keyboard_pressed.data(), 0, state->keyboard_pressed.size());
    std::memset(state->keyboard_released.data(), 0, state->keyboard_released.size());

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
            // fmt::println("down scan: {}, name: {}", (int)event.key.keysym.scancode,
            //   SDL_GetScancodeName(event.key.keysym.scancode));
            assert(event.key.keysym.scancode < MaxNumScancodes);
            state->keyboard_state[event.key.keysym.scancode] = true;
            state->keyboard_pressed[event.key.keysym.scancode] += 1;
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                return false;
            }
            break;
        case SDL_KEYUP:
            // fmt::println("up scan: {}, name: {}", (int)event.key.keysym.scancode,
            //   SDL_GetScancodeName(event.key.keysym.scancode));
            assert(event.key.keysym.scancode < MaxNumScancodes);
            state->keyboard_state[event.key.keysym.scancode] = false;
            state->keyboard_released[event.key.keysym.scancode] += 1;
            break;
        }
    }
    return true;
}

float get_time()
{
    return glwx::getTime();
}

uint64_t get_perf_counter()
{
    return SDL_GetPerformanceCounter();
}

uint64_t get_perf_counter_freq()
{
    return SDL_GetPerformanceFrequency();
}

float get_perf_counter_elapsed(uint64_t start, uint64_t factor)
{
    const auto delta = get_perf_counter() - start;
    return static_cast<float>(delta * factor) / static_cast<float>(get_perf_counter_freq());
}
}

struct Gfx {
    std::unique_ptr<glwx::SpriteRenderer> renderer;
    glm::mat4 projection;
    std::array<glw::Texture, 64> textures;

    static Gfx& instance()
    {
        static Gfx gfx;
        return gfx;
    }
};

namespace gfx {
void init()
{
    auto& plat = Platform::instance();
    auto& gfx = Gfx::instance();
    glw::State::instance().setViewport(plat.window.getSize().x, plat.window.getSize().y);
    gfx.renderer = std::make_unique<glwx::SpriteRenderer>();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gfx.projection = glm::ortho(0.0f, static_cast<float>(plat.window.getSize().x),
        static_cast<float>(plat.window.getSize().y), 0.0f);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void render_begin()
{
    auto& gfx = Gfx::instance();
    glClear(GL_COLOR_BUFFER_BIT);
    gfx.renderer->getDefaultShaderProgram().bind();
    gfx.renderer->getDefaultShaderProgram().setUniform("projectionMatrix", gfx.projection);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void render_end()
{
    Gfx::instance().renderer->flush();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    Platform::instance().window.swap();
}

Texture* load_texture(std::string_view path)
{
    auto& gfx = Gfx::instance();
    size_t tex_idx = 0;
    while (tex_idx < gfx.textures.size()
        && gfx.textures[tex_idx].getTarget() != glw::Texture::Target::Invalid) {
        tex_idx++;
    }
    assert(tex_idx < gfx.textures.size());
    auto tex = glwx::makeTexture2D(path, false);
    if (!tex) {
        return nullptr;
    }
    gfx.textures[tex_idx] = std::move(tex.value());
    return (Texture*)&gfx.textures[tex_idx];
}

void draw(const Texture* texture, float x, float y, float scale, float r, float g, float b, float a)
{
    auto tex = (const glw::Texture*)texture;
    assert(tex->getTarget() != glw::Texture::Target::Invalid);
    auto& renderer = Gfx::instance().renderer;
    const auto trafo = glwx::Transform2D(glm::vec2(x, y), 0.0f, glm::vec2(scale),
        glm::vec2(tex->getWidth(), tex->getHeight()) * -0.5f);
    renderer->color = glm::vec4(r, g, b, a);
    renderer->draw(*tex, trafo);
}
}
