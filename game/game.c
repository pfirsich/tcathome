#include <math.h>

#include "engine.h"

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    Vec2 pos;
    u32 sprite;
} Player;

typedef struct {
    Vec2 pos;
    float speed;
    float anger; // 0.0 = friendly, 1.0 = angry
    bool is_friendly; // true = can be eaten
    bool alive; // false = eaten
    u32 sprite;
} Chicken;

typedef struct {
    Player player;
    Chicken chickens[32];
    u32 chicken_count;
    u32 debug_circle_sprite;
} State;

const float SCREEN_W = 960;
const float SCREEN_H = 1080;
const float CHIK_AGGRO_RADIUS = 200.0f;

void* load()
{
    State* state = ng_alloc(sizeof(State));

    // Load sprites
    state->player.sprite = ng_load_image("assets/croc.png");
    state->debug_circle_sprite = ng_load_image("assets/circle.png");

    // Init player
    state->player.pos = (Vec2) { SCREEN_W / 2, SCREEN_H / 2 };

    // Init chickens
    state->chicken_count = 12;
    const u32 chicken_sprite = ng_load_image("assets/chik.png");
    for (int i = 0; i < state->chicken_count; i++) {
        const float spawn_margin = 100.0f;
        state->chickens[i].sprite = chicken_sprite;
        state->chickens[i].pos = (Vec2) {
            spawn_margin + ng_randomf() * (SCREEN_W - spawn_margin * 2.0f),
            spawn_margin + ng_randomf() * (SCREEN_W - spawn_margin * 2.0f),
        };
        state->chickens[i].speed = 150.0f + ng_randomf() * 100.0f;
        state->chickens[i].anger = 0.0f;
        state->chickens[i].is_friendly = (ng_randomf() > 0.5f);
        state->chickens[i].alive = true;
    }

    return state;
}

void update(State* state, float t, float dt)
{
    // Update player
    const int move_x = ng_is_key_down("d") - ng_is_key_down("a");
    const int move_y = ng_is_key_down("s") - ng_is_key_down("w");
    const float player_speed = 300.0f;

    state->player.pos.x += move_x * player_speed * dt;
    state->player.pos.y += move_y * player_speed * dt;

    // Update chickens
    for (int i = 0; i < state->chicken_count; i++) {
        Chicken* c = &state->chickens[i];
        if (!c->alive)
            continue;

        // Calculate distance to player
        float dx = state->player.pos.x - c->pos.x;
        float dy = state->player.pos.y - c->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < CHIK_AGGRO_RADIUS) {
            // Close enough to eat friendly chickens
            if (c->is_friendly) {
                c->alive = false;
                continue;
            }
            // Make unfriendly chickens angry
            c->anger = fminf(c->anger + dt * 2.0f, 1.0f);
        } else {
            // Cool down when far
            c->anger = fmaxf(c->anger - dt, 0.0f);
        }

        // Move towards player if angry
        if (c->anger > 0.0f) {
            float speed = c->speed * c->anger;
            c->pos.x += (dx / dist) * speed * dt;
            c->pos.y += (dy / dist) * speed * dt;
        }
    }
}

void render(const State* state)
{
    // Draw player
    ng_draw_sprite(state->player.sprite, state->player.pos.x, state->player.pos.y, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f);

    // Draw chickens
    for (int i = 0; i < state->chicken_count; i++) {
        const Chicken* c = &state->chickens[i];
        if (!c->alive)
            continue;

        if (true) {
            const float scale = CHIK_AGGRO_RADIUS / 256.0f;
            ng_draw_sprite(
                state->debug_circle_sprite, c->pos.x, c->pos.y, scale, 1.0f, 0.0f, 0.0f, 0.25f);
        }

        // Green for friendly, red for angry
        if (c->is_friendly) {
            ng_draw_sprite(c->sprite, c->pos.x, c->pos.y, 1.0f, 0.8f, 1.0f, 0.8f, 1.0f);
        } else {
            float r = 1.0f;
            float gb = 1.0f - c->anger;
            ng_draw_sprite(c->sprite, c->pos.x, c->pos.y, 1.0f, r, gb, gb, 1.0f);
        }
    }
}
