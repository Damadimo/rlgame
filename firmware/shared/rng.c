#include "rng.h"

static uint32_t next = 1u;

void game_rng_seed(uint32_t s)
{
    next = s;
}

int game_rand(void)
{
    next = next * 1103515245u + 12345u;
    return (int)((next >> 16) & 0x7fffu);
}
