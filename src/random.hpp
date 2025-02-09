#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>

// This small library is intended for *video games*.
// Don't use it if your cryptographic requirements are less than entirely non-existant.
// It uses SplitMix64 as a base and less-than perfect conversions to float.

namespace rng {

struct RandomState {
    uint64_t state;
};

RandomState* get_global_state();

void init_state(RandomState* state);

uint64_t random(RandomState* state = get_global_state());

// This tries to avoid modulo bias. If you are fine with modulo bias, just modulo yourself.
// This is not an entirely correct implementation, because `range` might overflow
// and if (max - min) does not fit into a u64, it will misbehave as well.
// Both `min' and `max` are inclusive
template <std::integral T>
T random(T min, T max, RandomState* state = get_global_state())
{
    assert(min <= max);
    if (min == max)
        return min;

    const auto range = static_cast<uint64_t>(max - min) + 1;
    const auto thresh = std::numeric_limits<uint64_t>::max() / range * range;

    while (true) {
        const auto r = random(state);
        if (r <= thresh) [[likely]] {
            return min + static_cast<T>(r % range);
        }
    }
}

float randomf(RandomState* state = get_global_state());
float randomf(float min, float max, RandomState* state = get_global_state());

double randomd(RandomState* state = get_global_state());
double randomd(double min, double max, RandomState* state = get_global_state());

}
