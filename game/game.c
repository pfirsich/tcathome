#include <math.h>

#include "engine.h"

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    Vec2 pos;
    u32 sprite;
} Player;

typedef struct {
    Vec2 pos;
    float speed;
    u32 sprite;
} Enemy;

typedef struct {
    Player player;
    Enemy enemies[32];
    u32 enemy_count;
} State;

void* load()
{
    const float res_x = 1600.0f;
    const float res_y = 900.0f;

    State* state = ng_alloc(sizeof(State));
    state->player.sprite = ng_load_image("assets/croc.png");
    state->player.pos = (Vec2) { res_x / 2.0f, res_y / 2.0f };

    u32 enemy_sprite = ng_load_image("assets/chiki.png");
    state->enemy_count = 8;
    for (int i = 0; i < state->enemy_count; i++) {
        state->enemies[i].sprite = enemy_sprite;
        state->enemies[i].pos = (Vec2) {
            ng_randomf() * res_x,
            ng_randomf() * res_y,
        };
        state->enemies[i].speed = 50 + ng_randomf() * 50;
    }

    return state;
}

void update(State* state, float t, float dt)
{
    const int move_x = ng_is_key_down("d") - ng_is_key_down("a");
    const int move_y = ng_is_key_down("s") - ng_is_key_down("w");
    const float speed = 200.0f;
    state->player.pos.x += move_x * speed * dt;
    state->player.pos.y += move_y * speed * dt;

    for (int i = 0; i < state->enemy_count; i++) {
        Enemy* e = &state->enemies[i];
        // Follow player
        const float dx = state->player.pos.x - e->pos.x;
        const float dy = state->player.pos.y - e->pos.y;
        const float len = sqrt(dx * dx + dy * dy);
        ng_break_if(len < 10.0f);
        e->pos.x += dx / len * e->speed * dt;
        e->pos.y += dy / len * e->speed * dt;
    }
}

void render(const State* state)
{
    for (int i = 0; i < state->enemy_count; i++) {
        const Enemy* e = &state->enemies[i];
        ng_draw_sprite(e->sprite, e->pos.x, e->pos.y);
    }

    ng_draw_sprite(state->player.sprite, state->player.pos.x, state->player.pos.y);
}
