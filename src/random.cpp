#include "random.hpp"

#include <random>

namespace rng {

void init_state(RandomState* state)
{
    std::random_device rd;
    state->state = rd();
}

RandomState* get_global_state()
{
    static RandomState state;
    return &state;
}

// clang-format off
/* SplitMix64
 * I spent so much time researching which algorithm to use considering the use case it doesn't
 * really matter.
 * The final round of contenders were:
 * - PCG: https://www.pcg-random.org/
 * - xoshiro256++: https://prng.di.unimi.it/
 * - SplitMix64
 * Here are some links that compare them:
 * - https://lemire.me/blog/2017/08/22/testing-non-cryptographic-random-number-generators-my-results/
 * - https://arvid.io/2018/07/02/better-cxx-prng/
 * - https://markus-seidl.de/unity-generalrandom/doc/rngs/
 * And after much deliberation I have come to the conclusion that they are all fine.
 * Their statistical properties are all surprisingly code given their complexity
 * and they are all certainly "good enough" for what I am doing with them.
 * I also don't need gigabytes of random data per second.
 * I chose splitmix because the implementation is very short and only has 64-bit state.
 */
// clang-format on
uint64_t random(RandomState* state)
{
    uint64_t z = (state->state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// I only do this because it's much better than float(rand())/max and it's easy to do
// You essentially fix the exponent to 2^0 (0x7f << 23) and then fill up the mantissa with
// some of the bits of the random number.
// The result is between [1, 2], so you have to subtract one.
float randomf(RandomState* state)
{
    const auto r = random(state);
    const uint32_t bits = (0x7ful << 23) | static_cast<uint32_t>(r >> (32 + 9));
    return std::bit_cast<float>(bits) - 1.0f;
}

// This is probably not great, but doing a generic version of the function above is troublesome
float randomf(float min, float max, RandomState* state)
{
    return min + randomf(state) * (max - min);
}

double randomd(RandomState* state)
{
    const auto r = random(state);
    const uint64_t bits = (0x3FFull << 52) | (r >> 12);
    return std::bit_cast<double>(bits) - 1.0;
}

double randomd(double min, double max, RandomState* state)
{
    return min + randomd(state) * (max - min);
}

}
