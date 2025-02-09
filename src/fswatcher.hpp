#pragma once

#include <string_view>

namespace fsw {
using ModifiedCallback = void(void* ctx, std::string_view path);

// path must point to a file
void add_watch(std::string_view path, ModifiedCallback* callback, void* ctx = nullptr);
void update();
}
