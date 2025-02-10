#pragma once

struct GameCode;

namespace gamecode {
GameCode* load(const char* path);
void* load(GameCode* gc);
// These two return whether they were broken from
bool update(GameCode* gc, void* s, float t, float dt);
bool render(GameCode* gc, const void* s);
void ng_break(); // only call this from an update/render callback!
}
