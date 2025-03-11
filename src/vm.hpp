#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "core.hpp"
#include "gamecode.hpp"
#include "random.hpp"

struct HotReloadState {
    // This provides a mapping from image handle to texture
    std::array<gfx::Texture*, 16> textures = {};
    GameCode* game_code;
};
static_assert(std::is_trivially_copyable_v<HotReloadState>);

struct EngineState : public HotReloadState {
    platform::InputState input_state;
    rng::RandomState random_state;
    float time;
    float dt;
};
static_assert(std::is_trivially_copyable_v<EngineState>);

struct Vm {
    enum class Mode { Advance, Pause, Playback, Replay };

    struct Error {
        const char* file;
        int line;
        std::string message;
    };

    static std::string_view to_string(Mode mode);

    EngineState engine_state; // The current engine and hot reload state
    uint32_t engine_state_track;
    void* state;
    HotReloadState hot_most_recent; // The most recent hot reload state
    uint32_t current_frame = 0;
    uint32_t last_frame = 0;
    std::optional<uint32_t> replay_mark;
    std::optional<uint64_t> stop_timestamp;
    uint32_t next_timestamp_id = 0;
    Mode mode = Mode::Advance;
    std::optional<Error> error;

    void init(const char* game_source);
    bool update();
    bool render();
    void update_time(float dt);
    void seek(uint32_t frame_id);
    void seek_timestamp(uint64_t ts);
    void copy_most_recent_hot_to_current();
    void save_next_frame();
    void set_input_state(const platform::InputState& state);
    void copy_input_state(uint32_t source_frame_id);
    uint64_t next_timestamp();
    bool update_advance(const platform::InputState& input_state, float dt);
    void finish_frame_advance();
    bool update_replay(float dt);
    void finish_frame_replay();
    bool update_playback();
    void finish_frame_playback();
    void start_advance();
    void start_pause();
    void start_replay(uint32_t start_frame_id);
    void start_playback();
};
