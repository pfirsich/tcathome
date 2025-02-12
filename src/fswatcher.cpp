#include "fswatcher.hpp"

#include <array>
#include <cassert>
#include <filesystem>

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace fsw {
struct Watch {
    std::string_view path;
    fs::file_time_type last_mod;
    ModifiedCallback* callback;
    void* ctx;
};

std::array<Watch, 128> watches = {};

void add_watch(std::string_view path, ModifiedCallback* callback, void* ctx)
{
    for (auto& watch : watches) {
        if (watch.path.empty()) {
            watch = Watch { path, fs::last_write_time(path), callback, ctx };
            break;
        }
    }
}

// All this is very inefficient and dumb, but easy to do

void update()
{
    for (auto& watch : watches) {
        if (watch.path.empty()) {
            break;
        }
        const auto mod = fs::last_write_time(watch.path);
        // This is not entirely good enough for renaming for example (would have to check inode on
        // Linux and no idea on Windows) So I just check inequality instead.
        if (mod != watch.last_mod) {
            fmt::println("modified: {}", watch.path);
            watch.last_mod = mod;
            watch.callback(watch.ctx, watch.path);
        }
    }
}
}
