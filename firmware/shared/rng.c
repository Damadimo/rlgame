// Small global PRNG for fruit placement, shared by solo and duel code.

#include "rng.h"

// Tiny global state for a classic linear congruential generator.
static uint32_t next = 1u;

// Reset the sequence, call when starting a new game or mode.
void game_rng_seed(uint32_t s)
{
    next = s;
}

int game_rand(void)
{
    // Same recurrence many libc rand implementations use.
    next = next * 1103515245u + 12345u;
    // Top bits have nicer distribution for our small ranges.
    return (int)((next >> 16) & 0x7fffu);
}
