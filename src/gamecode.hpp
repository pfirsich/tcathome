#pragma once

struct GameCode;

namespace gamecode {
GameCode* load(const char* path);
void* load(GameCode* gc);
void update(GameCode* gc, void* s, float t, float dt);
void render(GameCode* gc, const void* s);
}
